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
    
    FILE *fp = fopen(filename, "r");
    if(fp == NULL) {
        printf("Error: Could not open file %s\n", filename);
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    char *file_data = malloc(file_size);
    if(file_data == NULL) {
        printf("Error: Could not allocate memory for file data\n");
        fclose(fp);
        return 1;
    }
    size_t result = fread(file_data, file_size, 1, fp);
    if(result != 1) {
        printf("Error: Could not read file\n");
        free(file_data);
        fclose(fp);
        return 1;
    }

    int token_count = 0;
    Token* tokens = tokenize(file_data, &token_count);
    if (tokens != NULL) {
        print_tokens(tokens);
        free_tokens(tokens);
    };
    printf("Token count: %d\n", token_count);

    free(file_data);
    fclose(fp);
    return 0;
}
