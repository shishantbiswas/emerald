#include "ast.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

//=============================================================================
// Global Type Definitions
//=============================================================================

Type g_type_void = { .kind = TYPE_VOID, .size = 0 };
Type g_type_bool = { .kind = TYPE_BOOL, .size = 1 };
Type g_type_i32 = { .kind = TYPE_I32, .size = 4 };
Type g_type_i64 = { .kind = TYPE_I64, .size = 8 };
Type g_type_f32 = { .kind = TYPE_F32, .size = 4 };
Type g_type_f64 = { .kind = TYPE_F64, .size = 8 };
Type g_type_string = { .kind = TYPE_STRING, .size = 8 }; // Pointer size

//=============================================================================
// Parser State
//=============================================================================

typedef struct {
    const char *source;
    Token *tokens;
    size_t pos;
    size_t token_count;
    Arena *arena;
    int current_line;
    int current_column;
} Parser;

//=============================================================================
// Helper Functions
//=============================================================================

static Token *parser_current(Parser *parser) {
    if (parser->pos >= parser->token_count) {
        return &parser->tokens[parser->token_count - 1]; // Return EOF token
    }
    return &parser->tokens[parser->pos];
}

static Token *parser_advance(Parser *parser) {
    Token *tok = parser_current(parser);
    parser->pos++;
    return tok;
}

static bool parser_check(Parser *parser, Token_Type type) {
    return parser_current(parser)->type == type;
}

static bool parser_match(Parser *parser, Token_Type type) {
    if (parser_check(parser, type)) {
        parser_advance(parser);
        return true;
    }
    return false;
}

static void parser_expect(Parser *parser, Token_Type type, const char *message) {
    if (!parser_match(parser, type)) {
        fprintf(stderr, "Error at line %d: %s (got %s)\n",
                parser_current(parser)->line,
                message,
                parser_current(parser)->value ?: token_type_name(parser_current(parser)->type));
        exit(1);
    }
}

static void parser_error(Parser *parser, const char *message) {
    fprintf(stderr, "Error at line %d, column %d: %s\n",
            parser_current(parser)->line,
            parser_current(parser)->column,
            message);
    exit(1);
}

//=============================================================================
// Forward Declarations
//=============================================================================

static ASTNode *parse_statement(Parser *parser);
static ASTNode *parse_expression(Parser *parser);
static ASTBlock *parse_block(Parser *parser);
static Type *parse_type(Parser *parser);

//=============================================================================
// AST Node Creation
//=============================================================================

//=============================================================================
// Parsing Functions
//=============================================================================

// Parse type annotation
static Type *parse_type(Parser *parser) {
    
    if (parser_match(parser, TOKEN_I32)) return &g_type_i32;
    if (parser_match(parser, TOKEN_I64)) return &g_type_i64;
    if (parser_match(parser, TOKEN_F32)) return &g_type_f32;
    if (parser_match(parser, TOKEN_F64)) return &g_type_f64;
    if (parser_match(parser, TOKEN_STRING)) return &g_type_string;
    if (parser_match(parser, TOKEN_BOOL)) return &g_type_bool;
    
    // Default to i32 if no type specified
    return &g_type_i32;
}

// Parse identifier
static ASTIdentifier *parse_identifier(Parser *parser) {
    Token *tok = parser_current(parser);
    if (tok->type != TOKEN_IDENTIFIER) {
        parser_error(parser, "Expected identifier");
    }
    parser_advance(parser);

    ASTIdentifier *node = ast_new(parser->arena, ASTIdentifier);
    node->kind = AST_IDENTIFIER;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
    node->name = strdup(tok->value);
    return node;
}

// Parse literal
static ASTNode *parse_literal(Parser *parser) {
    Token *tok = parser_current(parser);

    if (tok->type == TOKEN_NUMBER) {
        // Check if it's float or int
        char *end;
        double float_val = strtod(tok->value, &end);
        
        if (*end == '.' || *end == 'e' || *end == 'E') {
            // It's a float
            ASTLiteralFloat *node = ast_new(parser->arena, ASTLiteralFloat);
    node->kind = AST_LITERAL_FLOAT;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
            node->value = float_val;
            node->type = &g_type_f64;
            parser_advance(parser);
            return (ASTNode*)node;
        } else {
            // It's an integer
            ASTLiteralInt *node = ast_new(parser->arena, ASTLiteralInt);
    node->kind = AST_LITERAL_INT;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
            node->value = strtoll(tok->value, NULL, 10);
            node->type = &g_type_i64;
            parser_advance(parser);
            return (ASTNode*)node;
        }
    }
    
    if (tok->type == TOKEN_STRING) {
        ASTLiteralString *node = ast_new(parser->arena, ASTLiteralString);
    node->kind = AST_LITERAL_STRING;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
        node->value = strdup(tok->value);
        node->type = &g_type_string;
        parser_advance(parser);
        return (ASTNode*)node;
    }
    
    if (tok->type == TOKEN_BOOL) {
        ASTLiteralBool *node = ast_new(parser->arena, ASTLiteralBool);
    node->kind = AST_LITERAL_BOOL;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
        node->value = (strcmp(tok->value, "true") == 0);
        node->type = &g_type_bool;
        parser_advance(parser);
        return (ASTNode*)node;
    }
    
    parser_error(parser, "Expected literal");
    return NULL;
}

// Parse primary expression (literals, identifiers, parenthesized)
static ASTNode *parse_primary(Parser *parser) {
    Token *tok = parser_current(parser);

    // Parenthesized expression
    if (parser_match(parser, TOKEN_LEFT_PAREN)) {
        ASTNode *expr = parse_expression(parser);
        parser_expect(parser, TOKEN_RIGHT_PAREN, "Expected ')'");
        return expr;
    }

    // Literal
    if (tok->type == TOKEN_NUMBER || tok->type == TOKEN_STRING || tok->type == TOKEN_BOOL) {
        return parse_literal(parser);
    }

    // Identifier
    if (tok->type == TOKEN_IDENTIFIER) {
        // Check if it's a function call
        ASTIdentifier *ident = parse_identifier(parser);
        
        if (parser_match(parser, TOKEN_LEFT_PAREN)) {
            // It's a function call
            ASTCallExpr *node = ast_new(parser->arena, ASTCallExpr);
    node->kind = AST_CALL_EXPR;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
            node->callee = (ASTNode*)ident;
            node->args = NULL;
            node->arg_count = 0;
            
            // Parse arguments
            if (!parser_check(parser, TOKEN_RIGHT_PAREN)) {
                do {
                    ASTNode *arg = parse_expression(parser);
                    node->args = realloc(node->args, sizeof(ASTNode*) * (node->arg_count + 1));
                    node->args[node->arg_count++] = arg;
                } while (parser_match(parser, TOKEN_COMMA));
            }
            
            parser_expect(parser, TOKEN_RIGHT_PAREN, "Expected ')'");
            return (ASTNode*)node;
        }
        
        return (ASTNode*)ident;
    }
    
    parser_error(parser, "Expected expression");
    return NULL;
}

// Parse unary expression
static ASTNode *parse_unary(Parser *parser) {
    Token *tok = parser_current(parser);

    if (parser_match(parser, TOKEN_EXCLAMATION)) {
        ASTNode *operand = parse_unary(parser);
        ASTUnaryExpr *node = ast_new(parser->arena, ASTUnaryExpr);
    node->kind = AST_UNARY_EXPR;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
        node->op = OP_NOT;
        node->operand = operand;
        return (ASTNode*)node;
    }
    
    if (parser_match(parser, TOKEN_OPERATOR) && 
        (strcmp(tok->value, "-") == 0 || strcmp(tok->value, "+") == 0)) {
        // Unary minus/plus
        ASTNode *operand = parse_unary(parser);
        ASTUnaryExpr *node = ast_new(parser->arena, ASTUnaryExpr);
    node->kind = AST_UNARY_EXPR;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
        node->op = (strcmp(tok->value, "-") == 0) ? OP_NEG : OP_NOT;
        node->operand = operand;
        return (ASTNode*)node;
    }
    
    if (parser_match(parser, TOKEN_TILDE)) {
        ASTNode *operand = parse_unary(parser);
        ASTUnaryExpr *node = ast_new(parser->arena, ASTUnaryExpr);
    node->kind = AST_UNARY_EXPR;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
        node->op = OP_BIT_NOT;
        node->operand = operand;
        return (ASTNode*)node;
    }
    
    if (parser_match(parser, TOKEN_QUESTION)) {
        // Null-coalescing or optional chaining - simplified for now
        ASTNode *operand = parse_unary(parser);
        ASTUnaryExpr *node = ast_new(parser->arena, ASTUnaryExpr);
    node->kind = AST_UNARY_EXPR;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
        node->op = OP_DEREF;
        node->operand = operand;
        return (ASTNode*)node;
    }
    
    return parse_primary(parser);
}

// Get binary operator from token
static BinaryOp get_binary_op(const char *op) {
    if (strcmp(op, "+") == 0) return OP_ADD;
    if (strcmp(op, "-") == 0) return OP_SUB;
    if (strcmp(op, "*") == 0) return OP_MUL;
    if (strcmp(op, "/") == 0) return OP_DIV;
    if (strcmp(op, "%") == 0) return OP_MOD;
    if (strcmp(op, "==") == 0) return OP_EQ;
    if (strcmp(op, "!=") == 0) return OP_NE;
    if (strcmp(op, "<") == 0) return OP_LT;
    if (strcmp(op, "<=") == 0) return OP_LE;
    if (strcmp(op, ">") == 0) return OP_GT;
    if (strcmp(op, ">=") == 0) return OP_GE;
    if (strcmp(op, "&&") == 0) return OP_AND;
    if (strcmp(op, "||") == 0) return OP_OR;
    if (strcmp(op, "&") == 0) return OP_BIT_AND;
    if (strcmp(op, "|") == 0) return OP_BIT_OR;
    if (strcmp(op, "^") == 0) return OP_BIT_XOR;
    if (strcmp(op, "<<") == 0) return OP_SHL;
    if (strcmp(op, ">>") == 0) return OP_SHR;
    return OP_ADD; // Default
}

// Parse binary expression (with precedence)
static ASTNode *parse_binary(Parser *parser, int min_prec) {
    ASTNode *left = parse_unary(parser);

    while (true) {
        Token *tok = parser_current(parser);

        if (tok->type != TOKEN_OPERATOR) {
            break;
        }
        
        // Get operator precedence (higher = binds tighter)
        int prec = 0;
        if (strcmp(tok->value, "||") == 0) prec = 1;
        else if (strcmp(tok->value, "&&") == 0) prec = 2;
        else if (strcmp(tok->value, "|") == 0) prec = 3;
        else if (strcmp(tok->value, "^") == 0) prec = 4;
        else if (strcmp(tok->value, "&") == 0) prec = 5;
        else if (strcmp(tok->value, "==") == 0 || strcmp(tok->value, "!=") == 0) prec = 6;
        else if (strcmp(tok->value, "<") == 0 || strcmp(tok->value, "<=") == 0 ||
                 strcmp(tok->value, ">") == 0 || strcmp(tok->value, ">=") == 0) prec = 7;
        else if (strcmp(tok->value, "<<") == 0 || strcmp(tok->value, ">>") == 0) prec = 8;
        else if (strcmp(tok->value, "+") == 0 || strcmp(tok->value, "-") == 0) prec = 9;
        else if (strcmp(tok->value, "*") == 0 || strcmp(tok->value, "/") == 0 || strcmp(tok->value, "%") == 0) prec = 10;
        else break;
        
        if (prec < min_prec) break;
        
        parser_advance(parser);
        
        BinaryOp op = get_binary_op(tok->value);
        ASTNode *right = parse_binary(parser, prec + 1);
        
        ASTBinaryExpr *node = ast_new(parser->arena, ASTBinaryExpr);
    node->kind = AST_BINARY_EXPR;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
        node->op = op;
        node->left = left;
        node->right = right;
        left = (ASTNode*)node;
    }
    
    return left;
}

// Parse assignment expression
static ASTNode *parse_assignment(Parser *parser) {
    ASTNode *left = parse_binary(parser, 1);
    Token *tok = parser_current(parser);

    if (tok->type == TOKEN_ASSIGN) {
        parser_advance(parser);
        ASTNode *right = parse_assignment(parser);
        
        ASTBinaryExpr *node = ast_new(parser->arena, ASTBinaryExpr);
    node->kind = AST_BINARY_EXPR;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
        node->op = OP_ASSIGN;
        node->left = left;
        node->right = right;
        return (ASTNode*)node;
    }
    
    // Compound assignments
    if (tok->type == TOKEN_OPERATOR) {
        BinaryOp op = OP_ADD; // Default
        if (strcmp(tok->value, "+=") == 0) op = OP_ADD_ASSIGN;
        else if (strcmp(tok->value, "-=") == 0) op = OP_SUB_ASSIGN;
        else if (strcmp(tok->value, "*=") == 0) op = OP_MUL_ASSIGN;
        else if (strcmp(tok->value, "/=") == 0) op = OP_DIV_ASSIGN;
        else return left;
        
        parser_advance(parser);
        ASTNode *right = parse_assignment(parser);
        
        // Convert compound assignment to: left = left op right
        ASTBinaryExpr *assign = ast_new(parser->arena, ASTBinaryExpr);
    assign->kind = AST_BINARY_EXPR;
    assign->type = NULL;
    assign->line = parser_current(parser)->line;
    assign->column = parser_current(parser)->column;
        BinaryOp base_op;
        switch (op) {
            case OP_ADD_ASSIGN: base_op = OP_ADD; break;
            case OP_SUB_ASSIGN: base_op = OP_SUB; break;
            case OP_MUL_ASSIGN: base_op = OP_MUL; break;
            case OP_DIV_ASSIGN: base_op = OP_DIV; break;
            default: base_op = OP_ADD;
        }
        assign->op = OP_ASSIGN;

        ASTBinaryExpr *bin_expr = ast_new(parser->arena, ASTBinaryExpr);
    bin_expr->kind = AST_BINARY_EXPR;
    bin_expr->type = NULL;
    bin_expr->line = parser_current(parser)->line;
    bin_expr->column = parser_current(parser)->column;
        bin_expr->op = base_op;
        bin_expr->left = left;
        bin_expr->right = right;
        assign->right = (ASTNode*)bin_expr;
        
        return (ASTNode*)assign;
    }
    
    return left;
}

// Parse expression
static ASTNode *parse_expression(Parser *parser) {
    return parse_assignment(parser);
}

// Parse block statement
static ASTBlock *parse_block(Parser *parser) {
    parser_expect(parser, TOKEN_LEFT_BRACE, "Expected '{'");
    
    ASTBlock *node = ast_new(parser->arena, ASTBlock);
    node->kind = AST_BLOCK;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
    node->statements = NULL;
    node->statement_count = 0;
    
    while (!parser_check(parser, TOKEN_RIGHT_BRACE) && !parser_check(parser, TOKEN_EOF)) {
        ASTNode *stmt = parse_statement(parser);
        if (stmt) {
            node->statements = realloc(node->statements, sizeof(ASTNode*) * (node->statement_count + 1));
            node->statements[node->statement_count++] = stmt;
        }
    }
    
    parser_expect(parser, TOKEN_RIGHT_BRACE, "Expected '}'");
    return node;
}

// Parse print statement
static ASTPrintStmt *parse_print(Parser *parser, bool is_println) {
    
    ASTPrintStmt *node = ast_new(parser->arena, ASTPrintStmt);
    node->kind = AST_PRINT_STMT;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
    node->is_println = is_println;
    node->args = NULL;
    node->arg_count = 0;
    
    parser_expect(parser, TOKEN_LEFT_PAREN, "Expected '('");
    
    if (!parser_check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            ASTNode *arg = parse_expression(parser);
            node->args = realloc(node->args, sizeof(ASTNode*) * (node->arg_count + 1));
            node->args[node->arg_count++] = arg;
        } while (parser_match(parser, TOKEN_COMMA));
    }
    
    parser_expect(parser, TOKEN_RIGHT_PAREN, "Expected ')'");
    parser_expect(parser, TOKEN_SEMICOLON, "Expected ';'");
    
    return node;
}

// Parse if statement
static ASTIfStmt *parse_if(Parser *parser) {
    
    ASTIfStmt *node = ast_new(parser->arena, ASTIfStmt);
    node->kind = AST_IF_STMT;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
    
    parser_expect(parser, TOKEN_LEFT_PAREN, "Expected '('");
    node->condition = parse_expression(parser);
    parser_expect(parser, TOKEN_RIGHT_PAREN, "Expected ')'");
    
    node->then_branch = parse_statement(parser);
    
    if (parser_match(parser, TOKEN_ELSE)) {
        node->else_branch = parse_statement(parser);
    } else {
        node->else_branch = NULL;
    }
    
    return node;
}


// Parse for statement
static ASTForStmt *parse_for(Parser *parser) {
    
    ASTForStmt *node = ast_new(parser->arena, ASTForStmt);
    node->kind = AST_FOR_STMT;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
    node->init = NULL;
    node->condition = NULL;
    node->update = NULL;
    
    // Simple for loop: for { body } or for (init; cond; update) { body }
    if (parser_match(parser, TOKEN_LEFT_BRACE)) {
        // Infinite loop with just body
        parser->pos--; // Put back the '{'
        node->body = parse_statement(parser);
        return node;
    }
    
    parser_expect(parser, TOKEN_LEFT_PAREN, "Expected '('");
    
    // Parse init (optional)
    if (!parser_check(parser, TOKEN_SEMICOLON)) {
        if (parser_match(parser, TOKEN_LET)) {
            // Variable declaration
            Token *name_tok = parser_current(parser);
            if (name_tok->type != TOKEN_IDENTIFIER) {
                parser_error(parser, "Expected variable name");
            }
            parser_advance(parser);
            
            ASTVariableDecl *node = ast_new(parser->arena, ASTVariableDecl);
    node->kind = AST_VARIABLE_DECL;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
            node->name = strdup(name_tok->value);
            node->is_mutable = false;
            node->var_type = &g_type_i32;

            if (parser_match(parser, TOKEN_ASSIGN)) {
                node->init = parse_expression(parser);
            } else {
                node->init = NULL;
            }
        } else {
            node->init = parse_expression(parser);
        }
    }
    parser_expect(parser, TOKEN_SEMICOLON, "Expected ';'");
    
    // Parse condition (optional)
    if (!parser_check(parser, TOKEN_SEMICOLON)) {
        node->condition = parse_expression(parser);
    }
    parser_expect(parser, TOKEN_SEMICOLON, "Expected ';'");
    
    // Parse update (optional)
    if (!parser_check(parser, TOKEN_RIGHT_PAREN)) {
        node->update = parse_expression(parser);
    }
    parser_expect(parser, TOKEN_RIGHT_PAREN, "Expected ')'");
    
    node->body = parse_statement(parser);
    
    return node;
}

// Parse return statement
static ASTReturnStmt *parse_return(Parser *parser) {
    
    ASTReturnStmt *node = ast_new(parser->arena, ASTReturnStmt);
    node->kind = AST_RETURN_STMT;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
    
    if (!parser_check(parser, TOKEN_SEMICOLON)) {
        node->value = parse_expression(parser);
    } else {
        node->value = NULL;
    }
    
    parser_expect(parser, TOKEN_SEMICOLON, "Expected ';'");
    return node;
}

// Parse variable declaration
static ASTVariableDecl *parse_variable_decl(Parser *parser) {
    
    Token *name_tok = parser_current(parser);
    if (name_tok->type != TOKEN_IDENTIFIER) {
        parser_error(parser, "Expected variable name");
    }
    parser_advance(parser);
    
    ASTVariableDecl *node = ast_new(parser->arena, ASTVariableDecl);
    node->kind = AST_VARIABLE_DECL;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
    node->name = strdup(name_tok->value);
    node->is_mutable = false;
    node->var_type = &g_type_i32;
    node->init = NULL;
    
    // Check for type annotation
    if (parser_match(parser, TOKEN_COLON)) {
        node->var_type = parse_type(parser);
    }
    
    // Check for initialization
    if (parser_match(parser, TOKEN_ASSIGN)) {
        node->init = parse_expression(parser);
    }
    
    parser_expect(parser, TOKEN_SEMICOLON, "Expected ';'");
    return node;
}

// Parse function declaration
static ASTFunction *parse_function(Parser *parser) {
    
    Token *name_tok = parser_current(parser);
    if (name_tok->type != TOKEN_IDENTIFIER) {
        parser_error(parser, "Expected function name");
    }
    parser_advance(parser);
    
    ASTFunction *node = ast_new(parser->arena, ASTFunction);
    node->kind = AST_FUNCTION;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
    node->name = strdup(name_tok->value);
    node->params = NULL;
    node->param_count = 0;
    node->is_exported = false;
    node->body = NULL;
    
    // Parse parameters
    parser_expect(parser, TOKEN_LEFT_PAREN, "Expected '('");

    if (!parser_check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            Token *param_name = parser_current(parser);
            if (param_name->type != TOKEN_IDENTIFIER) {
                parser_error(parser, "Expected parameter name");
            }
            parser_advance(parser);

            ASTParam *param = ast_new(parser->arena, ASTParam);
    param->kind = AST_PARAM;
    param->type = NULL;
    param->line = parser_current(parser)->line;
    param->column = parser_current(parser)->column;
            param->name = strdup(param_name->value);
            param->param_type = &g_type_i32;

            // Check for type annotation
            if (parser_match(parser, TOKEN_COLON)) {
                param->param_type = parse_type(parser);
            }

            node->params = realloc(node->params, sizeof(ASTNode*) * (node->param_count + 1));
            node->params[node->param_count++] = (ASTNode*)param;
        } while (parser_match(parser, TOKEN_COMMA));
    }
    
    parser_expect(parser, TOKEN_RIGHT_PAREN, "Expected ')'");

    // Parse return type
    Type *return_type = &g_type_i32;  // Default
    if (parser_match(parser, TOKEN_COLON)) {
        return_type = parse_type(parser);
    }

    // Create function type
    Type *func_type = arena_alloc_type(parser->arena, Type);
    func_type->kind = TYPE_FUNCTION;
    func_type->return_type = return_type;
    func_type->param_types = NULL;  // TODO: fill from params
    func_type->param_count = node->param_count;
    func_type->size = 0;
    func_type->base = NULL;
    node->func_type = func_type;

    // Parse function body or semicolon for extern
    if (parser_match(parser, TOKEN_SEMICOLON)) {
        node->body = NULL;  // Extern function declaration
    } else {
        ASTBlock *body = parse_block(parser);
        node->body = (ASTNode*)body;
    }
    
    return node;
}

// Parse statement
static ASTNode *parse_statement(Parser *parser) {
    
    // Empty statement
    if (parser_match(parser, TOKEN_SEMICOLON)) {
        return NULL;
    }
    
    // Block
    if (parser_check(parser, TOKEN_LEFT_BRACE)) {
        return (ASTNode*)parse_block(parser);
    }
    
    // Print / Println
    if (parser_match(parser, TOKEN_PRINT)) {
        return (ASTNode*)parse_print(parser, false);
    }
    
    // If
    if (parser_match(parser, TOKEN_IF)) {
        return (ASTNode*)parse_if(parser);
    }
    
    // For
    if (parser_match(parser, TOKEN_FOR)) {
        return (ASTNode*)parse_for(parser);
    }
    
    // Return
    if (parser_match(parser, TOKEN_RETURN)) {
        return (ASTNode*)parse_return(parser);
    }
    
    // Let (variable declaration)
    if (parser_match(parser, TOKEN_LET)) {
        return (ASTNode*)parse_variable_decl(parser);
    }
    
    // Function declaration
    if (parser_match(parser, TOKEN_FUNCTION)) {
        return (ASTNode*)parse_function(parser);
    }
    
    // Expression statement
    ASTNode *expr = parse_expression(parser);
    parser_expect(parser, TOKEN_SEMICOLON, "Expected ';'");
    
    ASTExprStmt *node = ast_new(parser->arena, ASTExprStmt);
    node->kind = AST_EXPR_STMT;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
    node->expr = expr;
    return (ASTNode*)node;
}

// Parse program
static ASTProgram *parse_program(Parser *parser) {
    ASTProgram *node = ast_new(parser->arena, ASTProgram);
    node->kind = AST_PROGRAM;
    node->type = NULL;
    node->line = parser_current(parser)->line;
    node->column = parser_current(parser)->column;
    node->imports = NULL;
    node->import_count = 0;
    node->exports = NULL;
    node->export_count = 0;
    node->functions = NULL;
    node->function_count = 0;
    
    while (!parser_check(parser, TOKEN_EOF)) {
        // Skip semicolons
        if (parser_match(parser, TOKEN_SEMICOLON)) {
            continue;
        }

        // Import
        if (parser_match(parser, TOKEN_IMPORT)) {
            ASTImport *imp = ast_new(parser->arena, ASTImport);
            imp->kind = AST_IMPORT;
            imp->type = NULL;
            imp->line = parser_current(parser)->line;
            imp->column = parser_current(parser)->column;
            Token *string_tok = parser_advance(parser);
            if (string_tok->type != TOKEN_STRING) {
                parser_error(parser, "Expected module name string after 'import'");
                return NULL;
            }
            imp->module_name = strdup(string_tok->value);
            parser_expect(parser, TOKEN_SEMICOLON, "Expected ';' after import statement");
            node->imports = realloc(node->imports, sizeof(ASTNode*) * (node->import_count + 1));
            node->imports[node->import_count++] = (ASTNode*)imp;
            continue;
        }

        // Export
        if (parser_match(parser, TOKEN_EXPORT)) {
            ASTExport *exp = ast_new(parser->arena, ASTExport);
            exp->kind = AST_EXPORT;
            exp->type = NULL;
            exp->line = parser_current(parser)->line;
            exp->column = parser_current(parser)->column;
            Token *ident_tok = parser_advance(parser);
            if (ident_tok->type != TOKEN_IDENTIFIER) {
                parser_error(parser, "Expected identifier after 'export'");
                return NULL;
            }
            ASTIdentifier *ident = ast_new(parser->arena, ASTIdentifier);
            ident->kind = AST_IDENTIFIER;
            ident->name = strdup(ident_tok->value);
            exp->target = (ASTNode*)ident;
            parser_expect(parser, TOKEN_SEMICOLON, "Expected ';' after export statement");
            node->exports = realloc(node->exports, sizeof(ASTNode*) * (node->export_count + 1));
            node->exports[node->export_count++] = (ASTNode*)exp;
            continue;
        }

        // Extern function
        if (parser_match(parser, TOKEN_EXTERN)) {
            if (!parser_match(parser, TOKEN_FUNCTION)) {
                parser_error(parser, "Expected 'function' after 'extern'");
                return NULL;
            }
            ASTNode *func = (ASTNode*)parse_function(parser);
            ((ASTFunction*)func)->is_extern = true;
            node->functions = realloc(node->functions, sizeof(ASTNode*) * (node->function_count + 1));
            node->functions[node->function_count++] = func;
            continue;
        }

        // Function
        if (parser_match(parser, TOKEN_FUNCTION)) {
            ASTNode *func = (ASTNode*)parse_function(parser);
            node->functions = realloc(node->functions, sizeof(ASTNode*) * (node->function_count + 1));
            node->functions[node->function_count++] = func;
            continue;
        }

        // Variable declaration (let x = 5;)
        if (parser_match(parser, TOKEN_LET)) {
            ASTNode *var_decl = (ASTNode*)parse_variable_decl(parser);
            // Top-level variable declarations become part of main function
            ASTFunction *main_func = NULL;
            // Look for existing main function
            for (size_t i = 0; i < node->function_count; i++) {
                if (((ASTFunction*)node->functions[i])->name && 
                    strcmp(((ASTFunction*)node->functions[i])->name, "main") == 0) {
                    main_func = (ASTFunction*)node->functions[i];
                    break;
                }
            }
            // If no main function, create one
            if (!main_func) {
                main_func = ast_new(parser->arena, ASTFunction);
                main_func->kind = AST_FUNCTION;
                main_func->name = strdup("main");
                main_func->param_count = 0;
                main_func->params = NULL;
                main_func->body = NULL;
                main_func->func_type = NULL;
                main_func->is_exported = false;
                main_func->is_extern = false;
                node->functions = realloc(node->functions, sizeof(ASTNode*) * (node->function_count + 1));
                node->functions[node->function_count++] = (ASTNode*)main_func;
            }
            // Add variable to main function's block
            if (main_func->body) {
                ASTBlock *block = (ASTBlock*)main_func->body;
                block->statements = realloc(block->statements, sizeof(ASTNode*) * (block->statement_count + 1));
                block->statements[block->statement_count++] = var_decl;
            } else {
                ASTBlock *block = ast_new(parser->arena, ASTBlock);
                block->kind = AST_BLOCK;
                block->statements = malloc(sizeof(ASTNode*));
                block->statements[0] = var_decl;
                block->statement_count = 1;
                main_func->body = (ASTNode*)block;
            }
            continue;
        }

        // Print statement
        if (parser_match(parser, TOKEN_PRINT)) {
            ASTNode *print_stmt = (ASTNode*)parse_print(parser, false);
            // Top-level print statements become part of main function
            ASTFunction *main_func = NULL;
            for (size_t i = 0; i < node->function_count; i++) {
                if (((ASTFunction*)node->functions[i])->name && 
                    strcmp(((ASTFunction*)node->functions[i])->name, "main") == 0) {
                    main_func = (ASTFunction*)node->functions[i];
                    break;
                }
            }
            if (!main_func) {
                main_func = ast_new(parser->arena, ASTFunction);
                main_func->kind = AST_FUNCTION;
                main_func->name = strdup("main");
                main_func->param_count = 0;
                main_func->params = NULL;
                main_func->body = NULL;
                main_func->func_type = NULL;
                main_func->is_exported = false;
                main_func->is_extern = false;
                node->functions = realloc(node->functions, sizeof(ASTNode*) * (node->function_count + 1));
                node->functions[node->function_count++] = (ASTNode*)main_func;
            }
            if (main_func->body) {
                ASTBlock *block = (ASTBlock*)main_func->body;
                block->statements = realloc(block->statements, sizeof(ASTNode*) * (block->statement_count + 1));
                block->statements[block->statement_count++] = print_stmt;
            } else {
                ASTBlock *block = ast_new(parser->arena, ASTBlock);
                block->kind = AST_BLOCK;
                block->statements = malloc(sizeof(ASTNode*));
                block->statements[0] = print_stmt;
                block->statement_count = 1;
                main_func->body = (ASTNode*)block;
            }
            continue;
        }

        // Error: unexpected token at top level
        parser_error(parser, "Expected declaration at top level");
        return NULL;
    }
    
    return node;
}

//=============================================================================
// Public API
//=============================================================================

ASTProgram *ast_parse(Arena *arena, const char *source) {
    Parser parser = {
        .source = source,
        .tokens = tokenize(source),
        .pos = 0,
        .token_count = 0,
        .arena = arena,
        .current_line = 1,
        .current_column = 1
    };
    
    // Count tokens
    while (parser.tokens[parser.token_count].type != TOKEN_EOF) {
        parser.token_count++;
    }
    parser.token_count++; // Include EOF

    ASTProgram *prog = parse_program(&parser);
    if (!prog) return NULL;

    // Check for main function
    bool has_main = false;
    for (size_t i = 0; i < prog->function_count; i++) {
        ASTFunction *func = (ASTFunction*)prog->functions[i];
        if (strcmp(func->name, "main") == 0) {
            has_main = true;
            break;
        }
    }

    if (!has_main) {
        fprintf(stderr, "Error: No main function found. Programs must have a 'main' function as the entry point.\n");
        return NULL;
    }

    return prog;
}

void ast_free(ASTProgram *prog) {
    // Not needed when using arena allocator
    (void)prog;
}

//=============================================================================
// Visitor Pattern Implementation
//=============================================================================

void ast_node_accept(ASTNode *node, ASTVisitor *visitor, void *data) {
    if (!node) return;
    
    switch (node->kind) {
        case AST_PROGRAM:
            if (visitor->visit_program) 
                visitor->visit_program((ASTProgram*)node, data);
            break;
        case AST_FUNCTION:
            if (visitor->visit_function) 
                visitor->visit_function((ASTFunction*)node, data);
            break;
        case AST_VARIABLE_DECL:
            if (visitor->visit_variable_decl) 
                visitor->visit_variable_decl((ASTVariableDecl*)node, data);
            break;
        case AST_BLOCK:
            if (visitor->visit_block) 
                visitor->visit_block((ASTBlock*)node, data);
            break;
        case AST_IF_STMT:
            if (visitor->visit_if_stmt)
                visitor->visit_if_stmt((ASTIfStmt*)node, data);
            break;
        case AST_FOR_STMT:
            if (visitor->visit_for_stmt) 
                visitor->visit_for_stmt((ASTForStmt*)node, data);
            break;
        case AST_RETURN_STMT:
            if (visitor->visit_return_stmt) 
                visitor->visit_return_stmt((ASTReturnStmt*)node, data);
            break;
        case AST_EXPR_STMT:
            if (visitor->visit_expr_stmt) 
                visitor->visit_expr_stmt((ASTExprStmt*)node, data);
            break;
        case AST_PRINT_STMT:
            if (visitor->visit_print_stmt) 
                visitor->visit_print_stmt((ASTPrintStmt*)node, data);
            break;
        case AST_BINARY_EXPR:
            if (visitor->visit_binary_expr) 
                visitor->visit_binary_expr((ASTBinaryExpr*)node, data);
            break;
        case AST_UNARY_EXPR:
            if (visitor->visit_unary_expr) 
                visitor->visit_unary_expr((ASTUnaryExpr*)node, data);
            break;
        case AST_CALL_EXPR:
            if (visitor->visit_call_expr) 
                visitor->visit_call_expr((ASTCallExpr*)node, data);
            break;
        case AST_INDEX_EXPR:
            if (visitor->visit_index_expr) 
                visitor->visit_index_expr((ASTIndexExpr*)node, data);
            break;
        case AST_MEMBER_EXPR:
            if (visitor->visit_member_expr) 
                visitor->visit_member_expr((ASTMemberExpr*)node, data);
            break;
        case AST_LITERAL_INT:
            if (visitor->visit_literal_int) 
                visitor->visit_literal_int((ASTLiteralInt*)node, data);
            break;
        case AST_LITERAL_FLOAT:
            if (visitor->visit_literal_float) 
                visitor->visit_literal_float((ASTLiteralFloat*)node, data);
            break;
        case AST_LITERAL_STRING:
            if (visitor->visit_literal_string) 
                visitor->visit_literal_string((ASTLiteralString*)node, data);
            break;
        case AST_LITERAL_BOOL:
            if (visitor->visit_literal_bool) 
                visitor->visit_literal_bool((ASTLiteralBool*)node, data);
            break;
        case AST_IDENTIFIER:
            if (visitor->visit_identifier) 
                visitor->visit_identifier((ASTIdentifier*)node, data);
            break;
        default:
            fprintf(stderr, "Unknown node kind: %d\n", node->kind);
            break;
    }
}

//=============================================================================
// Debug Printing
//=============================================================================

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
}

static void print_node(ASTNode *node, int indent);

static void print_program(ASTProgram *prog, int indent) {
    print_indent(indent);
    printf("Program (%zu functions)\n", prog->function_count);
    for (size_t i = 0; i < prog->function_count; i++) {
        print_node(prog->functions[i], indent + 1);
    }
}

static void print_function(ASTFunction *func, int indent) {
    print_indent(indent);
    printf("Function: %s\n", func->name);
    if (func->body) {
        print_node((ASTNode*)func->body, indent + 1);
    }
}

static void print_block(ASTBlock *block, int indent) {
    print_indent(indent);
    printf("Block (%zu statements)\n", block->statement_count);
    for (size_t i = 0; i < block->statement_count; i++) {
        print_node(block->statements[i], indent + 1);
    }
}

static void print_if(ASTIfStmt *if_stmt, int indent) {
    print_indent(indent);
    printf("If:\n");
    print_indent(indent + 1);
    printf("Condition:\n");
    print_node(if_stmt->condition, indent + 2);
    print_indent(indent + 1);
    printf("Then:\n");
    print_node(if_stmt->then_branch, indent + 2);
    if (if_stmt->else_branch) {
        print_indent(indent + 1);
        printf("Else:\n");
        print_node(if_stmt->else_branch, indent + 2);
    }
}


static void print_for(ASTForStmt *for_stmt, int indent) {
    print_indent(indent);
    printf("For:\n");
    if (for_stmt->init) {
        print_indent(indent + 1);
        printf("Init:\n");
        print_node(for_stmt->init, indent + 2);
    }
    if (for_stmt->condition) {
        print_indent(indent + 1);
        printf("Condition:\n");
        print_node(for_stmt->condition, indent + 2);
    }
    if (for_stmt->update) {
        print_indent(indent + 1);
        printf("Update:\n");
        print_node(for_stmt->update, indent + 2);
    }
    print_indent(indent + 1);
    printf("Body:\n");
    print_node(for_stmt->body, indent + 2);
}

static void print_return(ASTReturnStmt *ret, int indent) {
    print_indent(indent);
    printf("Return:\n");
    if (ret->value) {
        print_node(ret->value, indent + 1);
    }
}

static void print_expr_stmt(ASTExprStmt *expr_stmt, int indent) {
    print_indent(indent);
    printf("ExprStmt:\n");
    print_node(expr_stmt->expr, indent + 1);
}

static void print_print(ASTPrintStmt *print, int indent) {
    print_indent(indent);
    printf("Print%s (%zu args):\n", print->is_println ? "ln" : "", print->arg_count);
    for (size_t i = 0; i < print->arg_count; i++) {
        print_node(print->args[i], indent + 1);
    }
}

static void print_binary(ASTBinaryExpr *bin, int indent) {
    print_indent(indent);
    printf("BinaryOp: %s\n", ast_binary_op_name(bin->op));
    print_node(bin->left, indent + 1);
    print_node(bin->right, indent + 1);
}

static void print_unary(ASTUnaryExpr *unary, int indent) {
    print_indent(indent);
    printf("UnaryOp: %s\n", ast_unary_op_name(unary->op));
    print_node(unary->operand, indent + 1);
}

static void print_call(ASTCallExpr *call, int indent) {
    print_indent(indent);
    printf("Call (%zu args):\n", call->arg_count);
    print_node(call->callee, indent + 1);
    for (size_t i = 0; i < call->arg_count; i++) {
        print_node(call->args[i], indent + 1);
    }
}

static void print_var_decl(ASTVariableDecl *decl, int indent) {
    print_indent(indent);
    printf("VarDecl: %s\n", decl->name);
    if (decl->init) {
        print_node(decl->init, indent + 1);
    }
}

static void print_literal_int(ASTLiteralInt *lit, int indent) {
    print_indent(indent);
    printf("Int: %ld\n", lit->value);
}

static void print_literal_float(ASTLiteralFloat *lit, int indent) {
    print_indent(indent);
    printf("Float: %f\n", lit->value);
}

static void print_literal_string(ASTLiteralString *lit, int indent) {
    print_indent(indent);
    printf("String: \"%s\"\n", lit->value);
}

static void print_literal_bool(ASTLiteralBool *lit, int indent) {
    print_indent(indent);
    printf("Bool: %s\n", lit->value ? "true" : "false");
}

static void print_identifier(ASTIdentifier *ident, int indent) {
    print_indent(indent);
    printf("Ident: %s\n", ident->name);
}

static void print_node(ASTNode *node, int indent) {
    if (!node) {
        print_indent(indent);
        printf("<null>\n");
        return;
    }
    
    switch (node->kind) {
        case AST_PROGRAM:
            print_program((ASTProgram*)node, indent);
            break;
        case AST_FUNCTION:
            print_function((ASTFunction*)node, indent);
            break;
        case AST_BLOCK:
            print_block((ASTBlock*)node, indent);
            break;
        case AST_IF_STMT:
            print_if((ASTIfStmt*)node, indent);
            break;
        case AST_FOR_STMT:
            print_for((ASTForStmt*)node, indent);
            break;
        case AST_RETURN_STMT:
            print_return((ASTReturnStmt*)node, indent);
            break;
        case AST_EXPR_STMT:
            print_expr_stmt((ASTExprStmt*)node, indent);
            break;
        case AST_PRINT_STMT:
            print_print((ASTPrintStmt*)node, indent);
            break;
        case AST_BINARY_EXPR:
            print_binary((ASTBinaryExpr*)node, indent);
            break;
        case AST_UNARY_EXPR:
            print_unary((ASTUnaryExpr*)node, indent);
            break;
        case AST_CALL_EXPR:
            print_call((ASTCallExpr*)node, indent);
            break;
        case AST_VARIABLE_DECL:
            print_var_decl((ASTVariableDecl*)node, indent);
            break;
        case AST_LITERAL_INT:
            print_literal_int((ASTLiteralInt*)node, indent);
            break;
        case AST_LITERAL_FLOAT:
            print_literal_float((ASTLiteralFloat*)node, indent);
            break;
        case AST_LITERAL_STRING:
            print_literal_string((ASTLiteralString*)node, indent);
            break;
        case AST_LITERAL_BOOL:
            print_literal_bool((ASTLiteralBool*)node, indent);
            break;
        case AST_IDENTIFIER:
            print_identifier((ASTIdentifier*)node, indent);
            break;
        default:
            print_indent(indent);
            printf("<unknown node %d>\n", node->kind);
            break;
    }
}

void ast_print(ASTProgram *prog) {
    print_program(prog, 0);
}

//=============================================================================
// Name Functions
//=============================================================================

const char *ast_node_kind_name(ASTNodeKind kind) {
    switch (kind) {
        case AST_PROGRAM: return "Program";
        case AST_FUNCTION: return "Function";
        case AST_VARIABLE_DECL: return "VariableDecl";
        case AST_PARAM_LIST: return "ParamList";
        case AST_PARAM: return "Param";
        case AST_BLOCK: return "Block";
        case AST_IF_STMT: return "IfStmt";
        case AST_FOR_STMT: return "ForStmt";
        case AST_RETURN_STMT: return "ReturnStmt";
        case AST_EXPR_STMT: return "ExprStmt";
        case AST_PRINT_STMT: return "PrintStmt";
        case AST_BINARY_EXPR: return "BinaryExpr";
        case AST_UNARY_EXPR: return "UnaryExpr";
        case AST_CALL_EXPR: return "CallExpr";
        case AST_INDEX_EXPR: return "IndexExpr";
        case AST_MEMBER_EXPR: return "MemberExpr";
        case AST_LITERAL_INT: return "LiteralInt";
        case AST_LITERAL_FLOAT: return "LiteralFloat";
        case AST_LITERAL_STRING: return "LiteralString";
        case AST_LITERAL_BOOL: return "LiteralBool";
        case AST_IDENTIFIER: return "Identifier";
        case AST_BREAK_STMT: return "BreakStmt";
        case AST_CONTINUE_STMT: return "ContinueStmt";
        default: return "Unknown";
    }
}

const char *ast_binary_op_name(BinaryOp op) {
    switch (op) {
        case OP_ADD: return "+";
        case OP_SUB: return "-";
        case OP_MUL: return "*";
        case OP_DIV: return "/";
        case OP_MOD: return "%";
        case OP_EQ: return "==";
        case OP_NE: return "!=";
        case OP_LT: return "<";
        case OP_LE: return "<=";
        case OP_GT: return ">";
        case OP_GE: return ">=";
        case OP_AND: return "&&";
        case OP_OR: return "||";
        case OP_BIT_AND: return "&";
        case OP_BIT_OR: return "|";
        case OP_BIT_XOR: return "^";
        case OP_SHL: return "<<";
        case OP_SHR: return ">>";
        case OP_ASSIGN: return "=";
        case OP_ADD_ASSIGN: return "+=";
        case OP_SUB_ASSIGN: return "-=";
        case OP_MUL_ASSIGN: return "*=";
        case OP_DIV_ASSIGN: return "/=";
        default: return "?";
    }
}

const char *ast_unary_op_name(UnaryOp op) {
    switch (op) {
        case OP_NEG: return "-";
        case OP_NOT: return "!";
        case OP_BIT_NOT: return "~";
        case OP_DEREF: return "*";
        case OP_ADDR_OF: return "&";
        case OP_PRE_INC: return "++";
        case OP_PRE_DEC: return "--";
        case OP_POST_INC: return "++";
        case OP_POST_DEC: return "--";
        default: return "?";
    }
}
