#include "ast.h"
#include "token.h"
#include <stdio.h>
#include <string.h>

ASTNode* make_ast_program(Token *tokens) {
    ASTNode **statements = malloc(sizeof(ASTNode*) * 1024);
    if (statements == NULL) {
        return NULL;
    }
    
    int statement_count = 0;
    int pos = 0;
    
    while (tokens[pos].type != TOKEN_EOF) {
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
            
        case AST_FOR_LOOP:
            printf("FOR_LOOP (\n");
            if (node->left) {
                printf("  init: ");
                print_ast(node->left);
            }
            if (node->value.int_value == 0) {
                printf("  condition: <infinite loop>\n");
            }
            if (node->right) {
                printf("  body: ");
                print_ast(node->right);
            }
            printf(")\n");
            return;
            
        case AST_IF_STATEMENT:
            printf("IF_STATEMENT (\n");
            if (node->left) {
                printf("  condition: ");
                print_ast(node->left);
            }
            if (node->right) {
                printf("  body: ");
                print_ast(node->right);
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
            
        case AST_BINARY_OP:
            printf("BINARY_OP (%c, ", node->op);
            if (node->left) print_ast(node->left);
            printf(", ");
            if (node->right) print_ast(node->right);
            printf(")");
            return;
            
        case AST_UNARY_OP:
            printf("UNARY_OP (%c, ", node->op);
            if (node->left) print_ast(node->left);
            printf(")");
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

    // Handle if statement
    if (tokens[*pos].type == TOKEN_IF) {
        (*pos)++;  // Move past IF
        
        if (tokens[*pos].type != TOKEN_LEFT_PAREN) {
            fprintf(stderr, "Expected '(' after 'if' at position %d\n", *pos);
            return NULL;
        }
        (*pos)++;  // Move past '('
        
        // Create a new if node
        ASTNode *if_node = create_if_node(NULL);
        if (if_node == NULL) {
            return NULL;
        }
        
        // Handle the condition
        ASTNode *condition = parse_expression(tokens, pos);
        if (condition == NULL) { 
            free_ast(if_node);
            return NULL;
        }
        if_node->left = condition;  // Store condition as left child
        
        if (tokens[*pos].type != TOKEN_RIGHT_PAREN) {
            fprintf(stderr, "Expected ')' after if condition at position %d\n", *pos);
            free_ast(if_node);
            return NULL;
        }
        (*pos)++;  // Move past ')'
        
        if (tokens[*pos].type != TOKEN_LEFT_BRACE) {
            fprintf(stderr, "Expected '{' after if condition at position %d\n", *pos);
            free_ast(if_node);
            return NULL;
        }
        (*pos)++;  // Move past '{'
        
        // Handle the if block
        ASTNode *if_block = parse_statement(tokens, pos);
        if (if_block == NULL) {
            free_ast(if_node);
            return NULL;
        }
        if_node->right = if_block;  // Store if block as right child
        
        if (tokens[*pos].type != TOKEN_RIGHT_BRACE) {
            fprintf(stderr, "Expected '}' after if block at position %d\n", *pos);
            free_ast(if_node);
            return NULL;
        }
        (*pos)++;  // Move past '}'
        
        return if_node;
    }

    // Handle for loop
    if (tokens[*pos].type == TOKEN_FOR) {
        (*pos)++;  // Move past FOR
        
        if (tokens[*pos].type != TOKEN_LEFT_PAREN) {
            fprintf(stderr, "Expected '(' after 'for' at position %d\n", *pos);
            return NULL;
        }
        (*pos)++;  // Move past '('
        
        // Parse initialization (optional)
        ASTNode *init = NULL;
        if (tokens[*pos].type != TOKEN_SEMICOLON) {
            init = parse_expression(tokens, pos);
            if (init == NULL) {
                return NULL;
            }
        } else {
            (*pos)++;  // Move past ';'
        }
        
        // Parse condition (optional - if missing, it's an infinite loop)
        ASTNode *condition = NULL;
        if (tokens[*pos].type != TOKEN_SEMICOLON) {
            condition = parse_expression(tokens, pos);
            if (condition == NULL) {
                if (init) free_ast(init);
                return NULL;
            }
        }
        
        // The condition parsing should have stopped at the semicolon
        // If we're not at a semicolon, the condition parsing consumed too much
        if (tokens[*pos].type != TOKEN_SEMICOLON) {
            // Check if we're past the semicolon (condition parsing consumed it)
            // Look ahead to see if the next token is the update expression
            if (tokens[*pos].type == TOKEN_IDENTIFIER && tokens[*pos + 1].type == TOKEN_OPERATOR) {
                // We're at the update expression, which means the semicolon was consumed
                // This is okay, continue to the update parsing
            } else {
                fprintf(stderr, "Expected ';' after for condition at position %d, got %s\n", *pos, token_type_to_string(tokens[*pos].type));
                if (init) free_ast(init);
                if (condition) free_ast(condition);
                return NULL;
            }
        } else {
            (*pos)++;  // Move past ';'
        }
        
        // Parse update (optional)
        ASTNode *update = NULL;
        if (tokens[*pos].type != TOKEN_RIGHT_PAREN) {
            update = parse_expression(tokens, pos);
            if (update == NULL) {
                if (init) free_ast(init);
                if (condition) free_ast(condition);
                return NULL;
            }
        }
        
        if (tokens[*pos].type != TOKEN_RIGHT_PAREN) {
            fprintf(stderr, "Expected ')' after for update at position %d\n", *pos);
            if (init) free_ast(init);
            if (condition) free_ast(condition);
            if (update) free_ast(update);
            return NULL;
        }
        (*pos)++;  // Move past ')'
        
        if (tokens[*pos].type != TOKEN_LEFT_BRACE) {
            fprintf(stderr, "Expected '{' after for condition at position %d\n", *pos);
            if (init) free_ast(init);
            if (condition) free_ast(condition);
            if (update) free_ast(update);
            return NULL;
        }
        (*pos)++;  // Move past '{'
        
        // Handle the for loop body
        ASTNode *body = parse_statement(tokens, pos);
        if (body == NULL) {
            if (init) free_ast(init);
            if (condition) free_ast(condition);
            if (update) free_ast(update);
            return NULL;
        }
        
        if (tokens[*pos].type != TOKEN_RIGHT_BRACE) {
            fprintf(stderr, "Expected '}' after for body at position %d\n", *pos);
            if (init) free_ast(init);
            if (condition) free_ast(condition);
            if (update) free_ast(update);
            free_ast(body);
            return NULL;
        }
        (*pos)++;  // Move past '}'
        
        return create_for_loop_node(init, condition, update, body);
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

ASTNode* create_for_loop_node(ASTNode* init, ASTNode* condition, ASTNode* update, ASTNode* body) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL) return NULL;
    
    memset(node, 0, sizeof(ASTNode));  // Zero out the entire structure
    node->type = AST_FOR_LOOP;
    node->left = init;      // Store initialization in left child
    node->right = body;     // Store body in right child
    
    // Store condition and update in additional fields
    // For now, we'll use a simple approach and store them in the value union
    // In a more complete implementation, you might want to extend the ASTNode structure
    // to have more children or use a different approach
    if (condition) {
        node->value.int_value = 1;  // Indicate condition exists
    } else {
        node->value.int_value = 0;  // Indicate no condition (infinite loop)
    }
    
    // Store update information in the name field for now
    if (update) {
        strncpy(node->name, "has_update", sizeof(node->name) - 1);
    } else {
        strncpy(node->name, "no_update", sizeof(node->name) - 1);
    }
    
    return node;
}

ASTNode* create_if_node(ASTNode* condition) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL) return NULL;
    
    memset(node, 0, sizeof(ASTNode));  // Zero out the entire structure
    node->type = AST_IF_STATEMENT;
    node->left = condition;  // Store condition as left child
    node->right = NULL;      // Will store if block as right child
    
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
    if (tokens[*pos].type == TOKEN_NUMBER) {
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

    // Handle binary operations and unary operations
    if (tokens[*pos].type == TOKEN_IDENTIFIER) {
        ASTNode* left = malloc(sizeof(ASTNode));
        if (left == NULL) return NULL;
        
        left->type = AST_IDENTIFIER;
        left->left = NULL;
        left->right = NULL;
        strncpy(left->name, tokens[*pos].value, sizeof(left->name) - 1);
        left->name[sizeof(left->name) - 1] = '\0';
        left->value_type = VALUE_TYPE_IDENTIFIER;
        (*pos)++;
        
        // Check for binary operator
        if (tokens[*pos].type == TOKEN_OPERATOR) {
            char op = tokens[*pos].value[0];
            
            // Check for unary increment/decrement (++ or --)
            if (op == '+' && tokens[*pos + 1].type == TOKEN_OPERATOR && tokens[*pos + 1].value[0] == '+') {
                // Handle ++ (increment)
                (*pos) += 2;  // Skip both + tokens
                
                // Create unary operation node
                ASTNode* node = malloc(sizeof(ASTNode));
                if (node == NULL) {
                    free_ast(left);
                    return NULL;
                }
                
                node->type = AST_UNARY_OP;
                node->left = left;
                node->right = NULL;
                node->op = '+';  // Use + to indicate increment
                node->value_type = VALUE_TYPE_OPERATOR;
                
                return node;
            } else if (op == '-' && tokens[*pos + 1].type == TOKEN_OPERATOR && tokens[*pos + 1].value[0] == '-') {
                // Handle -- (decrement)
                (*pos) += 2;  // Skip both - tokens
                
                // Create unary operation node
                ASTNode* node = malloc(sizeof(ASTNode));
                if (node == NULL) {
                    free_ast(left);
                    return NULL;
                }
                
                node->type = AST_UNARY_OP;
                node->left = left;
                node->right = NULL;
                node->op = '-';  // Use - to indicate decrement
                node->value_type = VALUE_TYPE_OPERATOR;
                
                return node;
            } else {
                // Handle regular binary operator
                (*pos)++;
                
                // Parse right operand
                ASTNode* right = parse_expression(tokens, pos);
                if (right == NULL) {
                    free_ast(left);
                    return NULL;
                }
                
                // Create binary operation node
                ASTNode* node = malloc(sizeof(ASTNode));
                if (node == NULL) {
                    free_ast(left);
                    free_ast(right);
                    return NULL;
                }
                
                node->type = AST_BINARY_OP;
                node->left = left;
                node->right = right;
                node->op = op;
                node->value_type = VALUE_TYPE_OPERATOR;
                
                return node;
            }
        }
        
        return left;
    }
    
    // TODO: Add support for more expression types (unary ops, etc.)
    return NULL;
}