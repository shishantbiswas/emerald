#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

typedef enum {
    TOKEN_EOF, // End of file
    TOKEN_INT, // Integer 1-9
    TOKEN_FLOAT, // Float 1.0
    TOKEN_STRING, // String "Hello, World!"
    TOKEN_IDENTIFIER, // Identifier my_var
    TOKEN_PRINT,    // Keyword print
    TOKEN_IF,       // Keyword if
    TOKEN_ELSE,     // Keyword else
    TOKEN_LOOP,     // Keyword loop
    TOKEN_FOREACH,  // Keyword foreach
    TOKEN_WHILE,    // Keyword while
    TOKEN_RETURN,   // Keyword return
    TOKEN_LET,      // Keyword let
    TOKEN_OPERATOR, // Operator operator
    TOKEN_RIGHT_PAREN, // Right (
    TOKEN_LEFT_PAREN, // Left (
    TOKEN_RIGHT_BRACE, // Right {
    TOKEN_LEFT_BRACE, // Left {
    TOKEN_RIGHT_SQUARE, // Right [
    TOKEN_LEFT_SQUARE, // Left [
    TOKEN_SEMICOLON, // ;
    TOKEN_TILDE, // ~
    TOKEN_ASSIGN, // =
    TOKEN_COLON, // :
    TOKEN_COMMA, // ,
    TOKEN_DOT, // .
    TOKEN_QUESTION, // ?
    TOKEN_EXCLAMATION, // !
    TOKEN_COMMENT, // #
    TOKEN_WHITESPACE, // 
} Token_Type;

typedef struct Token {
    Token_Type type;
    char *value;
    size_t length;
    int line;
    int column;
} Token;

// Tokenizer functions
Token* tokenize(const char* input, int* token_count);
void free_tokens(Token* tokens);
void print_tokens(const Token* tokens);
char* token_type_to_string(Token_Type type);

#endif // TOKEN_H
