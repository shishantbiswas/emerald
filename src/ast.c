#include "ast.h"
#include "token.h"
#include <stdio.h>
#include <string.h>

ASTNode* make_ast_program(Token *tokens, int token_count, int *ast_count) {
    ASTNode **statements = malloc(sizeof(ASTNode*) * 1024);
    if (statements == NULL) {
        return NULL;
    }
    
    int statement_count = 0;
    int pos = 0;
    
    while (pos < token_count && tokens[pos].type != TOKEN_EOF) {
        ASTNode *stmt = parse_statement(tokens, &pos);
        if (stmt == NULL) {
            // Free any successfully parsed statements before returning
            for (int i = 0; i < statement_count; i++) {
                free_ast(statements[i]);
            }
            free(statements);
            return NULL;
        }
        statements[statement_count++] = stmt;
    }
    
    *ast_count = statement_count;
    return create_program_node(statements, statement_count);
}

void print_ast(ASTNode* node) {
    if (node == NULL) {
        return;
    }
    
    // Print current node
    switch (node->type) {
        case AST_PROGRAM:
            printf("PROGRAM (\n");
            ASTNode* current = node->left;
            while(current != NULL) {
                printf("  ");
                print_ast(current);
                current = current->right;
            }
            printf(")\n");
            return;
            
        case AST_PRINT:
            printf("PRINT (");
            if (node->left) {
                print_ast(node->left);
            } else if (node->value.string_value) {
                printf("\"%s\"", node->value.string_value);
            }
            printf(")\n");
            return;
            
        case AST_VARIABLE_DECLARATION:
            printf("VARIABLE_DECLARATION (%s = ", node->name);
            if (node->left) {
                print_ast(node->left);
            }
            printf(")\n");
            return;
            
        case AST_IDENTIFIER:
            printf("%s", node->name);
            return;
            
        case AST_LITERAL:
            switch (node->value_type) {
                case VALUE_TYPE_INT:
                    printf("%d", node->value.int_value);
                    break;
                case VALUE_TYPE_STRING:
                    printf("\"%s\"", node->value.string_value);
                    break;
                default:
                    printf("<unknown literal>");
            }
            return;
            
        default:
            printf("<unknown node type %d>", node->type);
            return;
    }
    
    // Print children
    if (node->left) {
        printf("  ");
        print_ast(node->left);
    }
    if (node->right) {
        printf("  ");
        print_ast(node->right);
    }
    
    printf(")\n");
}

ASTNode *parse_statement(Token *tokens, int *pos) {
    if (tokens[*pos].type == TOKEN_EOF) {
        return NULL;
    }

    // Handle print statement
    if (tokens[*pos].type == TOKEN_PRINT) {
        (*pos)++;  // Move past PRINT
        
        if (tokens[*pos].type != TOKEN_LEFT_PAREN) {
            fprintf(stderr, "Expected '(' after 'print' at position %d\n", *pos);
            return NULL;
        }
        (*pos)++;  // Move past '('
        
        // Create a new print node
        ASTNode *print_node = create_print_node(NULL);
        if (print_node == NULL) {
            return NULL;
        }
        
        // Handle the argument to print
        if (tokens[*pos].type == TOKEN_STRING) {
            print_node->value.string_value = strdup(tokens[*pos].value);
            print_node->value_type = VALUE_TYPE_STRING;
            (*pos)++;  // Move past the string
        } else {
            // Handle expressions in print
            ASTNode *expr = parse_expression(tokens, pos);
            if (expr == NULL) {
                free_ast(print_node);
                return NULL;
            }
            print_node->left = expr;  // Store expression as left child
        }
        
        if (tokens[*pos].type != TOKEN_RIGHT_PAREN) {
            fprintf(stderr, "Expected ')' after print argument at position %d\n", *pos);
            free_ast(print_node);
            return NULL;
        }
        (*pos)++;  // Move past ')'
        
        if (tokens[*pos].type != TOKEN_SEMICOLON) {
            fprintf(stderr, "Expected ';' after print statement at position %d\n", *pos);
            free_ast(print_node);
            return NULL;
        }
        (*pos)++;  // Move past ';'
        
        return print_node;
    }
    
    // Handle variable declaration
    if (tokens[*pos].type == TOKEN_LET) {
        (*pos)++;  // Move past LET
        
        if (tokens[*pos].type != TOKEN_IDENTIFIER) {
            fprintf(stderr, "Expected identifier after 'let' at position %d\n", *pos);
            return NULL;
        }
        
        char *name = strdup(tokens[*pos].value);
        if (name == NULL) {
            return NULL;
        }
        (*pos)++;  // Move past identifier
        
        if (tokens[*pos].type != TOKEN_ASSIGN) {
            fprintf(stderr, "Expected '=' in variable declaration at position %d\n", *pos);
            free(name);
            return NULL;
        }
        (*pos)++;  // Move past '='
        
        ASTNode *value = parse_expression(tokens, pos);
        if (value == NULL) {
            free(name);
            return NULL;
        }
        
        if (tokens[*pos].type != TOKEN_SEMICOLON) {
            fprintf(stderr, "Expected ';' after variable declaration at position %d\n", *pos);
            free(name);
            free_ast(value);
            return NULL;
        }
        (*pos)++;  // Move past ';'
        
        return create_var_declare_node(name, value);
    }
    
    // Try to parse as an expression statement
    ASTNode *expr = parse_expression(tokens, pos);
    if (expr != NULL) {
        if (tokens[*pos].type == TOKEN_SEMICOLON) {
            (*pos)++;  // Consume the semicolon
            return expr;
        }
        free_ast(expr);
    }
    
    return NULL;
}

ASTNode* create_print_node(char* value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL) return NULL;
    
    memset(node, 0, sizeof(ASTNode));  // Zero out the entire structure
    node->type = AST_PRINT;
    node->left = NULL;
    node->right = NULL;
    node->value_type = VALUE_TYPE_STRING;
    node->value.string_value = value ? strdup(value) : NULL;
    
    return node;
}

// Function to free an AST
void free_ast(ASTNode* node) {
    if (node == NULL) return;
    
    free_ast(node->left);
    free_ast(node->right);
    
    if (node->type == AST_PRINT && node->value.string_value)
        free(node->value.string_value);

    free(node);
}

ASTNode* create_var_declare_node(const char* name, ASTNode* value) {
    if (name == NULL) {
        return NULL;
    }
    
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL) return NULL;
    
    memset(node, 0, sizeof(ASTNode));  // Zero out the entire structure
    node->type = AST_VARIABLE_DECLARATION;
    node->left = value;  // Store the value in left child
    node->right = NULL;
    strncpy(node->name, name, sizeof(node->name) - 1);
    node->name[sizeof(node->name) - 1] = '\0';  // Ensure null termination
    node->value_type = VALUE_TYPE_IDENTIFIER;
    
    return node;
}

ASTNode* create_program_node(ASTNode** statements, int statement_count) {
    if (statements == NULL) {
        return NULL;
    }
    
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL) {
        return NULL;
    }
    
    memset(node, 0, sizeof(ASTNode));  // Zero out the entire structure
    node->type = AST_PROGRAM;
    node->left = NULL;
    node->right = NULL;
    
    // For now, we'll just use the first statement as the left child
    // In a more complete implementation, you might want to create a proper
    // program node structure that can hold multiple statements
    if (statement_count > 0) {
        node->left = statements[0];
        
        // Link remaining statements as a linked list in the right pointers
        ASTNode* current = node->left;
        for (int i = 1; i < statement_count; i++) {
            current->right = statements[i];
            current = current->right;
        }
    }
    
    // Free the statements array but not the nodes themselves
    // as they are now part of the AST
    free(statements);
    
    return node;
}

ASTNode* parse_expression(Token* tokens, int *pos) {
    // Handle identifiers
    if (tokens[*pos].type == TOKEN_IDENTIFIER) {
        ASTNode* node = malloc(sizeof(ASTNode));
        if (node == NULL) return NULL;
        
        node->type = AST_IDENTIFIER;
        node->left = NULL;
        node->right = NULL;
        strncpy(node->name, tokens[*pos].value, sizeof(node->name) - 1);
        node->name[sizeof(node->name) - 1] = '\0';
        node->value_type = VALUE_TYPE_IDENTIFIER;
        (*pos)++;
        return node;
    }
    
    // Handle string literals
    if (tokens[*pos].type == TOKEN_STRING) {
        ASTNode* node = malloc(sizeof(ASTNode));
        if (node == NULL) return NULL;
        
        node->type = AST_LITERAL;
        node->left = NULL;
        node->right = NULL;
        node->value.string_value = strdup(tokens[*pos].value);
        node->value_type = VALUE_TYPE_STRING;
        (*pos)++;
        return node;
    }
    
    // Handle integer literals
    if (tokens[*pos].type == TOKEN_INT) {
        ASTNode* node = malloc(sizeof(ASTNode));
        if (node == NULL) return NULL;
        
        node->type = AST_LITERAL;
        node->left = NULL;
        node->right = NULL;
        node->value.int_value = atoi(tokens[*pos].value);
        node->value_type = VALUE_TYPE_INT;
        (*pos)++;
        return node;
    }
    
    // TODO: Add support for more expression types (binary ops, unary ops, etc.)
    return NULL;
}