#ifndef AST_H
#define AST_H

#include <stdlib.h>

typedef enum {
    AST_PROGRAM,
    AST_EXPRESSION,
    AST_STATEMENT,
    AST_IDENTIFIER,
    AST_LITERAL,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_CALL,
    AST_FUNCTION,
    AST_VARIABLE_DECLARATION,
    AST_IF_STATEMENT,
    AST_WHILE_LOOP,
    AST_RETURN,
    AST_PRINT
} AST_Node_Type;

typedef struct AST_Node {
    AST_Node_Type type;
    struct AST_Node* left;
    struct AST_Node* right;
    char* value;
} AST_Node;

// Function declarations
AST_Node* create_ast_node(AST_Node_Type type);
void free_ast(AST_Node* node);

#endif