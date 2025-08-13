#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

typedef enum {
    TOKEN_EOF, // End of file
    TOKEN_INT, // Integer 1-9
    TOKEN_FLOAT, // Float 1.0
    TOKEN_STRING, // String "Hello, World!"
    TOKEN_IDENTIFIER, // Identifier my_var
    TOKEN_KEYWORD, // Keyword let, if, else, while, return, print
    TOKEN_OPERATOR, // Operator operator
    TOKEN_RIGHT_PAREN, // Right (
    TOKEN_LEFT_PAREN, // Left (
    TOKEN_RIGHT_BRACE, // Right {
    TOKEN_LEFT_BRACE, // Left {
    TOKEN_RIGHT_SQUARE, // Right [
    TOKEN_LEFT_SQUARE, // Left [
    TOKEN_SEMICOLON, // ;
    TOKEN_TILDE, // ~
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

#endif // TOKEN_H
