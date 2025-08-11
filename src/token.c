#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "token.h"

// Forward declarations
int is_operator(char c);
int is_punctuation(char c);

Token* tokenize(const char* input) {
    if (input == NULL) return NULL;
    
    Token *tokens = malloc(sizeof(Token) * 1024);
    if (tokens == NULL) return NULL;
    
    int token_count = 0;
    int i = 0;
    int line = 1;
    int column = 1;
    
    while (input[i] != '\0' && token_count < 1024) {
        // Skip whitespace
        if (isspace(input[i])) {
            if (input[i] == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            i++;
            continue;
        }
        
        // Start of a new token
        tokens[token_count].line = line;
        tokens[token_count].column = column;
        
        // Handle identifiers and keywords
        if (isalpha(input[i]) || input[i] == '_') {
            int start = i;
            while (isalnum(input[i]) || input[i] == '_') {
                i++;
                column++;
            }
            int length = i - start;
            tokens[token_count].type = TOKEN_IDENTIFIER;
            tokens[token_count].value = strndup(input + start, length);
            tokens[token_count].length = length;
            
            // Check if it's a keyword
            if (strncmp(tokens[token_count].value, "print", 5) == 0) {
                tokens[token_count].type = TOKEN_KEYWORD;
            }
        }
        // Handle strings
        else if (input[i] == '"') {
            i++;
            column++;
            int start = i;
            while (input[i] != '"' && input[i] != '\0') {
                i++;
                column++;
            }
            if (input[i] == '"') {
                int length = i - start;
                tokens[token_count].type = TOKEN_STRING;
                tokens[token_count].value = strndup(input + start, length);
                tokens[token_count].length = length;
                i++;
                column++;
            } else {
                // Unclosed string
                fprintf(stderr, "Error: Unclosed string at line %d, column %d\n", line, column);
                return NULL;
            }
        }
        // Handle operators
        else if (is_operator(input[i])) {
            tokens[token_count].type = TOKEN_OPERATOR;
            tokens[token_count].value = strndup(input + i, 1);
            tokens[token_count].length = 1;
            i++;
            column++;
        }
        // Handle punctuation
        else if (is_punctuation(input[i])) {
            tokens[token_count].type = TOKEN_PUNCTUATION;
            tokens[token_count].value = strndup(input + i, 1);
            tokens[token_count].length = 1;
            i++;
            column++;
        }
        // Skip comments for now
        else if (input[i] == '/' && input[i+1] == '/') {
            while (input[i] != '\n' && input[i] != '\0') {
                i++;
                column++;
            }
            continue;
        }
        // Handle unknown characters
        else {
            fprintf(stderr, "Error: Unknown character '%c' at line %d, column %d\n", 
                   input[i], line, column);
            i++;
            column++;
            continue;
        }
        
        token_count++;
    }
    
    // Add EOF token
    tokens[token_count].type = TOKEN_EOF;
    tokens[token_count].value = NULL;
    tokens[token_count].length = 0;
    tokens[token_count].line = line;
    tokens[token_count].column = column;
    
    return tokens;
}

void free_tokens(Token* tokens) {
    if (tokens == NULL) return;
    
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        if (tokens[i].value != NULL) {
            free(tokens[i].value);
        }
    }
}

int is_operator(char c) {
    switch (c) {
        case '+': case '-': case '*': case '/':
        case '=': case '<': case '>': case '!':
        case '&': case '|': case '^': case '%':
            return 1;
        default:
            return 0;
    }
}

int is_punctuation(char c) {
    switch (c) {
        case '(': case ')': case '{': case '}':
        case '[': case ']': case ';': case ',':
        case '.': case ':': case '?': case '~':
            return 1;
        default:
            return 0;
    }
}

// Helper function to print token type as string
const char* token_type_to_string(Token_Type type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_INT: return "INT";
        case TOKEN_FLOAT: return "FLOAT";
        case TOKEN_STRING: return "STRING";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_KEYWORD: return "KEYWORD";
        case TOKEN_OPERATOR: return "OPERATOR";
        case TOKEN_PUNCTUATION: return "PUNCTUATION";
        case TOKEN_COMMENT: return "COMMENT";
        case TOKEN_WHITESPACE: return "WHITESPACE";
        default: return "UNKNOWN";
    }
}

// Example usage
void print_tokens(const Token* tokens) {
    if (tokens == NULL) {
        printf("No tokens to print.\n");
        return;
    }
    
    printf("Tokens:\n");
    printf("%-15s %-15s %-10s %-10s %s\n", 
           "Type", "Value", "Line", "Column", "Length");
    printf("--------------------------------------------------\n");
    
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        const char* value = tokens[i].value ? tokens[i].value : "(null)";
        printf("%-15s %-15s %-10d %-10d %zu\n",
               token_type_to_string(tokens[i].type),
               value,
               tokens[i].line,
               tokens[i].column,
               tokens[i].length);
    }
    printf("%-15s %-15s %-10d %-10d %d\n",
           token_type_to_string(TOKEN_EOF), 
           "(null)", 
           tokens[0].line, 
           tokens[0].column, 
           0);
}
