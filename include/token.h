#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

typedef enum {
    TOKEN_EOF,          // End of file

    // Integer types
    TOKEN_I8,           // singed 8 bit integer
    TOKEN_I16,          // singed 16 bit integer
    TOKEN_I32,          // singed 32 bit integer
    TOKEN_I64,          // singed 64 bit integer
    TOKEN_I128,         // singed 128 bit integer
    TOKEN_U8,           // unsigned 8 bit integer
    TOKEN_U16,          // unsigned 16 bit integer
    TOKEN_U32,          // unsigned 32 bit integer
    TOKEN_U64,          // unsigned 64 bit integer
    TOKEN_U128,         // unsigned 128 bit integer

    // Float types
    TOKEN_F8,           // Float 8 bit
    TOKEN_F16,          // Float 16 bit
    TOKEN_F32,          // Float 32 bit
    TOKEN_F64,          // Float 64 bit
    TOKEN_F128,         // Float 128 bit

    TOKEN_NUMBER,       // Number for value

    // Boolean types
    TOKEN_BOOL,         // Boolean true or false

    TOKEN_STRING,       // String "Hello, World!"
    TOKEN_IDENTIFIER,   // Identifier my_var

    // Keywords
    TOKEN_PRINT,        // Keyword print
    TOKEN_IF,           // Keyword if
    TOKEN_ELSE,         // Keyword else
    TOKEN_FOREACH,      // Keyword foreach
    TOKEN_FUNCTION,     // Keyword function
    TOKEN_FOR,          // Keyword for
    TOKEN_WHILE,        // Keyword while
    TOKEN_RETURN,       // Keyword return
    TOKEN_LET,          // Keyword let
    TOKEN_MUT,          // Keyword mut
    TOKEN_CONST,        // Keyword const

    // Operators
    TOKEN_COLON,        // Colon : for type annotations
    TOKEN_OPERATOR,     // Operator operator
    TOKEN_RIGHT_PAREN,  // Right (
    TOKEN_LEFT_PAREN,   // Left (
    TOKEN_RIGHT_BRACE,  // Right {
    TOKEN_LEFT_BRACE,   // Left {
    TOKEN_RIGHT_SQUARE, // Right [
    TOKEN_LEFT_SQUARE,  // Left [
    TOKEN_SEMICOLON,    // ;
    TOKEN_TILDE,        // ~
    TOKEN_ASSIGN,       // =
    TOKEN_COMMA,        // ,
    TOKEN_DOT,          // .
    TOKEN_QUESTION,     // ?
    TOKEN_EXCLAMATION,  // !
    TOKEN_COMMENT,      // #
    TOKEN_WHITESPACE,   // 
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
char* token_type_to_string(Token_Type type);

#endif // TOKEN_H
