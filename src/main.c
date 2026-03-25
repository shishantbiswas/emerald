#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"
#include "ast.h"
#include "ir.h"
#include "codegen.h"
#include "arena.h"

typedef struct {
    bool debug_mode;
    bool release_mode;
    bool verbose;
    bool check_qbe;
    char *ir_output_file;
    char *exe_output_file;
    char *filename;
} BuildConfig;

int main(int argc, char *argv[]) {
    char *filename = NULL;
    char *ir_output_file = "build/ir.ssa";
    char *exe_output_file = NULL;
    bool verbose = false;
    bool check_qbe = false;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            ir_output_file = argv[++i];
        } else if (strcmp(argv[i], "-c") == 0) {
            if (i + 1 < argc) {
                exe_output_file = argv[++i];
            } else {
                exe_output_file = "build/a.out";
            }
        } else if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "-qbe") == 0) {
            check_qbe = true;
        } else if (!filename && argv[i][0] != '-') {
            filename = argv[i];
        }
    }

    // Handle QBE check
    if (check_qbe) {
        if (codegen_check_qbe_available()) {
            printf("QBE is available: %s\n", codegen_get_qbe_version());
            return 0;
        } else {
            printf("QBE is not available on this system\n");
            return 1;
        }
    }

    // Check if filename is provided
    if (!filename) {
        printf("Compiler for Emerald Programming Language\n");
        printf("Usage: %s <FILE> [OPTIONS]\n", argv[0]);
        printf("Options:\n");
        printf("  -o <OUTPUT>  Specify IR output file (default: build/ir.ssa)\n");
        printf("  -c <OUTPUT>  Compile to executable (default: build/a.out)\n");
        printf("  -v           Enable verbose output (print AST)\n");
        printf("  -qbe         Check QBE availability\n");
        return 1;
    }

    // Validate filename
    if (strlen(filename) == 0 || strlen(filename) > 1024) {
        printf("Error: Invalid filename length\n");
        return 1;
    }
    for (const char *p = filename; *p; p++) {
        if (*p < 32 || *p == 127) { // control characters
            printf("Error: Invalid characters in filename\n");
            return 1;
        }
    }
    
    // Open file
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error: Could not open file %s\n", filename);
        return 1;
    }
    
    // Get file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);
    
    // Allocate memory for file data
    char *file_data = malloc(file_size + 1);
    if (file_data == NULL) {
        printf("Error: Could not allocate memory for file data\n");
        fclose(fp);
        return 1;
    }
    
    // Read file
    size_t result = fread(file_data, file_size, 1, fp);
    if (result != 1) {
        perror("Error: Could not read file");
        free(file_data);
        fclose(fp);
        return 1;
    }
    file_data[file_size] = '\0';
    fclose(fp);
    
    // Create arena allocator
    Arena *arena = arena_create(64 * 1024); // 64KB blocks
    if (arena == NULL) {
        printf("Error: Could not create arena allocator\n");
        free(file_data);
        return 1;
    }
    
    // Parse
    printf("Parsing...\n");
    ASTProgram *ast = ast_parse(arena, file_data);
    
    if (ast == NULL) {
        printf("Error: Failed to parse\n");
        arena_destroy(arena);
        free(file_data);
        return 1;
    }
    
    // Print AST if verbose
    if (verbose) {
        printf("\n=== Generated AST ===\n");
        ast_print(ast);
        printf("====================\n\n");
    }
    
    // Generate IR
    printf("Generating IR...\n");
    IRModule *module = ir_module_create(arena, "main");
    int ir_result = ir_generate(module, ast);
    
    if (ir_result != 0) {
        printf("Error: Failed to generate IR\n");
        arena_destroy(arena);
        free(file_data);
        return 1;
    }
    
    // Emit QBE IR
    printf("Emitting QBE IR to %s...\n", ir_output_file);
    int emit_result = ir_emit(module, ir_output_file);
    
    if (emit_result != 0) {
        printf("Error: Failed to emit IR\n");
        arena_destroy(arena);
        free(file_data);
        return 1;
    }
    
    printf("Success!\n");

    // Compile to executable if requested
    if (exe_output_file) {
        printf("Building executable...\n");
        int codegen_result = codegen_build_executable(ir_output_file, exe_output_file);
        if (codegen_result != 0) {
            printf("Error: Failed to build executable\n");
            arena_destroy(arena);
            free(file_data);
            return 1;
        }
        printf("Executable built: %s\n", exe_output_file);
    }

    // Cleanup
    arena_destroy(arena);
    free(file_data);

    return 0;
}
