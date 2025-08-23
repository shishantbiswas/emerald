#ifndef AST_H
#define AST_H

#include <stdlib.h>
#include "token.h"

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
    AST_PRINT,
    AST_VAR,
} ASTNode_Type;

typedef union {
    int int_value;
    float float_value;
    char *string_value;
} ASTValue;

typedef struct ASTNode {
    ASTNode_Type type;
    struct ASTNode* left;
    struct ASTNode* right;
    char name[64];
    char op;
    enum ValueType {
        VALUE_TYPE_INT,
        VALUE_TYPE_FLOAT,
        VALUE_TYPE_STRING,
        VALUE_TYPE_IDENTIFIER,
        VALUE_TYPE_KEYWORD,
        VALUE_TYPE_OPERATOR,
        VALUE_TYPE_RIGHT_PAREN,
        VALUE_TYPE_LEFT_PAREN,
        VALUE_TYPE_RIGHT_BRACE,
        VALUE_TYPE_LEFT_BRACE,
        VALUE_TYPE_SEMICOLON,
        VALUE_TYPE_COLON,
        VALUE_TYPE_COMMA,
        VALUE_TYPE_DOT,
        VALUE_TYPE_QUESTION,
        VALUE_TYPE_EXCLAMATION,
        VALUE_TYPE_COMMENT,
        VALUE_TYPE_WHITESPACE,
    } value_type;
    ASTValue value;
} ASTNode;

// Function declarations
ASTNode* make_ast_program(Token* tokens, int token_count, int *ast_count);
ASTNode* create_print_node(char* value);
void print_ast(ASTNode* node);
void free_ast(ASTNode* node);

// AST Node creation functions
ASTNode* create_print_node(char* value);
ASTNode* create_var_declare_node(const char* name, ASTNode* value);
ASTNode* create_program_node(ASTNode** statements, int statement_count);

// Parsing functions
ASTNode* parse_statement(Token* tokens, int* pos);
ASTNode* parse_expression(Token* tokens, int* pos);

// Utility functions
void print_ast(ASTNode* node);
void free_ast(ASTNode* node);

#endif