#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

typedef enum {
    TOKEN_EOF,
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD,
    TOKEN_OPERATOR,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_BRACE,
    TOKEN_SEMICOLON,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_QUESTION,
    TOKEN_EXCLAMATION,
    TOKEN_COMMENT,
    TOKEN_WHITESPACE,
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
