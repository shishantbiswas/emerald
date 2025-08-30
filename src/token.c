#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "token.h"

// Forward declarations
int is_operator(char c);
int is_punctuation(char c);
void handle_identifier_and_keyword(Token* tokens, int token_index, const char* input, int* i, int* column);
void handle_punctuation(Token* tokens, int token_index, const char* input, int* i, int* column);
void handle_string(Token* tokens, int token_index, const char* input, int* i, int* column);

Token* tokenize(const char* input) {
    if (input == NULL) return NULL;
    
    Token *tokens = malloc(sizeof(Token) * 1024);
    if (tokens == NULL) return NULL;
    
    int token_index = 0;
    int i = 0;
    int line = 1;
    int column = 1;
    
    while (input[i] != '\0' && token_index < 1024) {
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
        tokens[token_index].line = line;
        tokens[token_index].column = column;
        
        // Handle identifiers and keywords
        if (isalpha(input[i]) || input[i] == '_') {
            handle_identifier_and_keyword(tokens, token_index, input, &i, &column);
        }
        // Handle strings
        else if (input[i] == '"') {
            handle_string(tokens, token_index, input, &i, &column);
        }
        
    // Handle int
    else if (isdigit(input[i])) {
        int start = i;
        while (isdigit(input[i])) {
            i++;
            column++;
        }
        int length = i - start;
        tokens[token_index].type = TOKEN_NUMBER;
        tokens[token_index].value = strndup(input + start, length);
        tokens[token_index].length = length;
    }

        // Handle operators
        else if (is_operator(input[i])) {
            if (input[i] == '=') {
                tokens[token_index].type = TOKEN_ASSIGN;
            } else {
                tokens[token_index].type = TOKEN_OPERATOR;
            }
            tokens[token_index].value = strndup(input + i, 1);
            tokens[token_index].length = 1;
            i++;
            column++;
        }
        // Handle punctuation
        else if (is_punctuation(input[i])) {
            handle_punctuation(tokens, token_index, input, &i, &column);
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
        
        token_index++;
    }
    
    // Add EOF token
    tokens[token_index].type = TOKEN_EOF;
    tokens[token_index].value = NULL;
    tokens[token_index].length = 0;
    tokens[token_index].line = line;
    tokens[token_index].column = column;
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
char* token_type_to_string(Token_Type type) {
    switch (type) {
        case TOKEN_EOF:         return "EOF";
        case TOKEN_NUMBER:      return "NUMBER";
        case TOKEN_STRING:      return "STRING";
        case TOKEN_IDENTIFIER:  return "IDENTIFIER";
        case TOKEN_PRINT:       return "PRINT";
        case TOKEN_BOOL:        return "BOOL";
        case TOKEN_IF:          return "IF";
        case TOKEN_ELSE:        return "ELSE";
        case TOKEN_FOREACH:     return "FOREACH";
        case TOKEN_FOR:         return "FOR";
        case TOKEN_WHILE:       return "WHILE";
        case TOKEN_RETURN:      return "RETURN";
        case TOKEN_LET:         return "LET";
        case TOKEN_ASSIGN:      return "ASSIGN";
        case TOKEN_OPERATOR:    return "OPERATOR";
        case TOKEN_RIGHT_PAREN: return "RIGHT_PAREN";
        case TOKEN_LEFT_PAREN:  return "LEFT_PAREN";
        case TOKEN_RIGHT_BRACE: return "RIGHT_BRACE";
        case TOKEN_LEFT_BRACE:  return "LEFT_BRACE";
        case TOKEN_SEMICOLON:   return "SEMICOLON";
        case TOKEN_COLON:       return "COLON";
        case TOKEN_COMMA:       return "COMMA";
        case TOKEN_DOT:         return "DOT";
        case TOKEN_QUESTION:    return "QUESTION";
        case TOKEN_EXCLAMATION: return "EXCLAMATION";
        case TOKEN_COMMENT:     return "COMMENT";
        case TOKEN_WHITESPACE:  return "WHITESPACE";
        default:                return "UNKNOWN";
    }
}

// Example usage
void print_tokens(const Token* tokens) {
    if (tokens == NULL) {
        printf("No tokens to print.\n");
        return;
    }
    
    printf("Tokens:\n");
    printf("%-20s %-20s %-10s %-10s %s\n", 
           "Type", "Value", "Line", "Column", "Length");
    printf("-----------------------------------------------------------------------\n");
    
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        const char* value = tokens[i].value;

        char str_val[16];
        if (tokens[i].value != NULL) {
            size_t len = strlen(tokens[i].value);
            if (len > 10) {
                strncpy(str_val, tokens[i].value, 10);
                str_val[10] = '.';
                str_val[11] = '.';
                str_val[12] = '.';
                str_val[13] = '\0';
            } else {
                strncpy(str_val, tokens[i].value, sizeof(str_val) - 1);
                str_val[sizeof(str_val) - 1] = '\0';
            }
        }
        
        printf("%-20s %-20s %-10d %-10d %zu\n",
               token_type_to_string(tokens[i].type),
               value ? str_val : "(null)",
               tokens[i].line,
               tokens[i].column,
               tokens[i].length);
    }
    printf("%-20s %-20s %-10d %-10d %d\n",
           token_type_to_string(TOKEN_EOF), 
           "(null)", 
           tokens[0].line, 
           tokens[0].column, 
           0);
}

// Handle identifier and keywords
void handle_identifier_and_keyword(Token* tokens, int token_index, const char* input, int* i, int* column) {
    int start = *i;
    while (isalnum(input[*i]) || input[*i] == '_') {
        (*i)++;
        (*column)++;
    }
    int length = *i - start;
    tokens[token_index].type = TOKEN_IDENTIFIER;
    tokens[token_index].value = strndup(input + start, length);
    tokens[token_index].length = length;
        
    // Check if it's a keyword
    if (strcmp(tokens[token_index].value, "print") == 0) {
        tokens[token_index].type = TOKEN_PRINT;
    } else if (strcmp(tokens[token_index].value, "if") == 0) {
        tokens[token_index].type = TOKEN_IF;
    } else if (strcmp(tokens[token_index].value, "else") == 0) {
        tokens[token_index].type = TOKEN_ELSE;
    } else if (strcmp(tokens[token_index].value, "foreach") == 0) {
        tokens[token_index].type = TOKEN_FOREACH;
    } else if (strcmp(tokens[token_index].value, "for") == 0) {
        tokens[token_index].type = TOKEN_FOR;
    } else if (strcmp(tokens[token_index].value, "while") == 0) {
        tokens[token_index].type = TOKEN_WHILE;
    }else if (strcmp(tokens[token_index].value, "let") == 0) {
        tokens[token_index].type = TOKEN_LET;
    } else if (strcmp(tokens[token_index].value, "mut") == 0) {
        tokens[token_index].type = TOKEN_MUT;
    } else if (strcmp(tokens[token_index].value, "const") == 0) {
        tokens[token_index].type = TOKEN_CONST;
    } else if (strcmp(tokens[token_index].value, "function") == 0) {
        tokens[token_index].type = TOKEN_FUNCTION;
    } else if (strcmp(tokens[token_index].value, "return") == 0) {
        tokens[token_index].type = TOKEN_RETURN;
    } else if (strcmp(tokens[token_index].value, "true") == 0) {
        tokens[token_index].type = TOKEN_BOOL;
    } else if (strcmp(tokens[token_index].value, "false") == 0) {
        tokens[token_index].type = TOKEN_BOOL;
    }
}

// Handle punctuation
void handle_punctuation(Token* tokens, int token_index, const char* input, int* i, int* column) {
    if (input[*i] == '(') {
        tokens[token_index].type = TOKEN_LEFT_PAREN;
    } else if (input[*i] == ')') {
        tokens[token_index].type = TOKEN_RIGHT_PAREN;
    } else if (input[*i] == '{') {
        tokens[token_index].type = TOKEN_LEFT_BRACE;
    } else if (input[*i] == '}') {
        tokens[token_index].type = TOKEN_RIGHT_BRACE;
    } else if (input[*i] == ';') {
        tokens[token_index].type = TOKEN_SEMICOLON;
    } else if (input[*i] == ',') {
        tokens[token_index].type = TOKEN_COMMA;
    } else if (input[*i] == '.') {
        tokens[token_index].type = TOKEN_DOT;
    } else if (input[*i] == ':') {
        tokens[token_index].type = TOKEN_COLON;
    } else if (input[*i] == '?') {
        tokens[token_index].type = TOKEN_QUESTION;
    } else if (input[*i] == '!') {
        tokens[token_index].type = TOKEN_EXCLAMATION;
    } else if (input[*i] == '~') {
        tokens[token_index].type = TOKEN_TILDE;
    }
    tokens[token_index].value = strndup(input + *i, 1);
    tokens[token_index].length = 1;
    (*i)++;
    (*column)++;
}

// Handle string
void handle_string(Token* tokens, int token_index, const char* input, int* i, int* column) {    
    (*i)++;
    (*column)++;
    int start = *i;
    while (input[*i] != '"' && input[*i] != '\0') {
        (*i)++;
        (*column)++;
    }
    if (input[*i] == '"') {
        int length = *i - start;
        tokens[token_index].type = TOKEN_STRING;
        tokens[token_index].value = strndup(input + start, length);
        tokens[token_index].length = length;
        (*i)++;
        (*column)++;
    } else {
        fprintf(stderr, "Error: Unclosed string at line %d, column %d\n", tokens[token_index].line, tokens[token_index].column);
    }
}
