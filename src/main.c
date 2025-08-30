#include <stdio.h>
#include <stdlib.h>
#include "token.h"
#include "ir.h"
#include "ast.h"

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Usage: %s <FILE>\n", argv[0]);
        return 1;
    }
    char *filename = argv[1];

    // Open file
    FILE *fp = fopen(filename, "r");
    if(fp == NULL) {
        printf("Error: Could not open file %s\n", filename);
        return 1;
    }
    // Get file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    // Allocate memory for file data
    char *file_data = malloc(file_size);
    if(file_data == NULL) {
        printf("Error: Could not allocate memory for file data\n");
        fclose(fp);
        return 1;
    }

    // Read file
    size_t result = fread(file_data, file_size, 1, fp);
    if(result != 1) {
        printf("Error: Could not read file\n");
        free(file_data);
        fclose(fp);
        return 1;
    }
    
    // Tokenize
    Token* tokens = tokenize(file_data);
    #ifdef DEBUG
    print_tokens(tokens);
    #endif
    
    // Parse
    ASTNode* ast = make_ast_program(tokens);
    
    // Debug output
    #ifdef DEBUG
    if (ast == NULL) {
        printf("Error: Failed to create AST\n");
    } else {
        print_ast(ast);
    }
    #endif

    // Make IR
    // int ir_success = make_ir(ast);
    // if(ir_success != 0) {
    //     printf("Error: Could not make IR\n");
    // }
    
    // Free memory
    if (ast != NULL) {
        free_ast(ast);
    }

    if (tokens != NULL) {
        free_tokens(tokens);
    };
    
    free(file_data);
    fclose(fp);
    return 0;
}
