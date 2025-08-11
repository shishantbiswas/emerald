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
    TOKEN_PUNCTUATION,
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
Token* tokenize(const char* input);
void free_tokens(Token* tokens);
void print_tokens(const Token* tokens);

#endif // TOKEN_H
