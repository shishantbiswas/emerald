#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

//=============================================================================
// Helper Functions
//=============================================================================

// Safer command execution using fork/exec instead of popen to avoid shell interpretation
static int run_command(const char *command, char **output, size_t *output_size) {
    // WARNING: Simple parsing: split command into args (space separated, no quotes)
    // NOTE: This is fragile - args[] point into cmd_copy which is freed in parent after fork.
    // This works due to fork's copy-on-write semantics, but should not be modified without
    // understanding the implications. Consider using execv with pre-parsed args array.
    char *cmd_copy = strdup(command);
    if (!cmd_copy) return -1;

    char *args[64]; // Max 64 args
    int arg_count = 0;
    char *token = strtok(cmd_copy, " ");
    while (token && arg_count < 63) {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL;

    if (arg_count == 0) {
        free(cmd_copy);
        return -1;
    }

    int pipefd[2];
    if (output && pipe(pipefd) == -1) {
        free(cmd_copy);
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        free(cmd_copy);
        if (output) close(pipefd[0]), close(pipefd[1]);
        return -1;
    }

    if (pid == 0) { // Child
        if (output) {
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
        }
        execvp(args[0], args);
        _exit(127); // Exec failed
    } else { // Parent
        free(cmd_copy);
        if (output) close(pipefd[1]);

        int status;
        if (output) {
            char buffer[4096];
            *output = NULL;
            *output_size = 0;
            ssize_t n;
            while ((n = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
                *output = realloc(*output, *output_size + n + 1);
                if (!*output) {
                    close(pipefd[0]);
                    waitpid(pid, &status, 0);
                    return -1;
                }
                memcpy(*output + *output_size, buffer, n);
                *output_size += n;
            }
            (*output)[*output_size] = '\0';
            close(pipefd[0]);
        }

        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return -1;
        }
    }
}

static int run_command_quiet(const char *command) {
    return run_command(command, NULL, NULL);
}

//=============================================================================
// Public API
//=============================================================================

int codegen_compile_qbe(const char *ssa_file, const char *obj_file) {
    if (!ssa_file || !obj_file) {
        fprintf(stderr, "Error: NULL file paths provided to codegen_compile_qbe\n");
        return -1;
    }

    // Check if QBE is available
    if (!codegen_check_qbe_available()) {
        fprintf(stderr, "Error: QBE is not available on this system\n");
        return -1;
    }

    // Construct QBE command: qbe -o output.o input.ssa
    char command[1024];
    snprintf(command, sizeof(command), "qbe -o %s %s", obj_file, ssa_file);

    printf("Compiling with QBE: %s -> %s\n", ssa_file, obj_file);

    int result = run_command_quiet(command);
    if (result != 0) {
        fprintf(stderr, "Error: QBE compilation failed with exit code %d\n", result);
        fprintf(stderr, "Command: %s\n", command);
        return result;
    }

    return 0;
}

int codegen_link(const char *obj_file, const char *output_file) {
    if (!obj_file || !output_file) {
        fprintf(stderr, "Error: NULL file paths provided to codegen_link\n");
        return -1;
    }

    // Link with standard libraries
    // For simplicity, we'll use gcc for linking since it handles the standard library
    char command[1024];
    snprintf(command, sizeof(command), "gcc -o %s %s -static", output_file, obj_file);

    printf("Linking: %s -> %s\n", obj_file, output_file);

    int result = run_command_quiet(command);
    if (result != 0) {
        fprintf(stderr, "Error: Linking failed with exit code %d\n", result);
        fprintf(stderr, "Command: %s\n", command);
        return result;
    }

    return 0;
}

int codegen_build_executable(const char *ssa_file, const char *output_file) {
    if (!ssa_file || !output_file) {
        fprintf(stderr, "Error: NULL file paths provided to codegen_build_executable\n");
        return -1;
    }

    // Create temporary object file name
    char obj_file[256];
    snprintf(obj_file, sizeof(obj_file), "%s.o", output_file);

    // Step 1: Compile QBE to object file
    int result = codegen_compile_qbe(ssa_file, obj_file);
    if (result != 0) {
        return result;
    }

    // Step 2: Link object file to executable
    result = codegen_link(obj_file, output_file);
    if (result != 0) {
        // Clean up object file on failure
        unlink(obj_file);
        return result;
    }

    // Clean up object file on success
    unlink(obj_file);

    printf("Successfully built executable: %s\n", output_file);
    return 0;
}

bool codegen_check_qbe_available(void) {
    int result = run_command_quiet("which qbe");
    return result == 0;
}

const char *codegen_get_qbe_version(void) {
    static char version[256] = {0};

    if (version[0] != '\0') {
        return version;
    }

    char *output = NULL;
    size_t output_size = 0;

    int result = run_command("qbe --version 2>&1", &output, &output_size);
    if (result == 0 && output) {
        // Extract first line
        char *newline = strchr(output, '\n');
        if (newline) {
            *newline = '\0';
        }
        strncpy(version, output, sizeof(version) - 1);
        version[sizeof(version) - 1] = '\0';
        free(output);
        return version;
    }

    if (output) {
        free(output);
    }

    return "QBE not available";
}
