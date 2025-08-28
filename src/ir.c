#include "ir.h"
#include "ast.h"
#include "hash_table.h"
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int create_build_dirs();

int make_ir(ASTNode* ast) {

    if(ast == NULL) {
        return -1;
    }

    if(create_build_dirs() != 0) {
        return -1;
    }

    char file_name[] = "build/ir.ssa";
    FILE *fp = fopen(file_name, "w");
    if(fp == NULL) {
        printf("Error: Could not open file %s\n", file_name);
        return -1;
    }

    Hashtable* string_table = createHashtable(128);
    if(string_table == NULL) {
        printf("Error: Could not create string table\n");
        return -1;
    }

    ASTNode* current = ast->left;
    while(current != NULL) {
        if(current->type == AST_PRINT) {
            insertEntry(string_table, current->value.string_value);
        }
        current = current->right;
    }

    // First pass: output all string constants with hashed labels
    for(int i = 0; i < string_table->size; i++) {
        Entry* current = string_table->table[i];
        while(current != NULL) {
            unsigned int h = hash(current->data);
            fprintf(fp, "data $str_%u = { b \"%s\", b 0 }\n", h, current->data);
            current = current->next;
        }
    }

    fprintf(fp, "\nexport function w $main() {\n@start\n");
    ASTNode* current_node = ast->left;
    while (current_node != NULL) {
        if (current_node->type == AST_PRINT) {
            // Use hash of the string as the label
            unsigned int h = hash(current_node->value.string_value);
            fprintf(fp, "    call $puts(l $str_%u)\n", h);
        }
        current_node = current_node->right;
    }
    fprintf(fp, "    ret 0\n}");

    freeHashtable(string_table);
    fclose(fp);

    return 0;
}

int create_build_dirs(){
    char dir_name[] = "build";
    struct stat st = {0};
    if (stat(dir_name, &st) != 0) {
        if (mkdir(dir_name, 0700) == 0) {
            printf("Build directory '%s' created successfully.\n", dir_name);
        } else {
            perror("Error creating build directory");
            return -1;
        }
    }

    return 0;
}