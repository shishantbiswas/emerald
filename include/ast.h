#ifndef EMERALD_AST_H
#define EMERALD_AST_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "arena.h"

//=============================================================================
// Type Definitions
//=============================================================================

// Forward declarations
typedef struct ASTNode ASTNode;
typedef struct ASTProgram ASTProgram;
typedef struct ASTStatement ASTStatement;
typedef struct ASTExpression ASTExpression;

// Type kinds for the type system
typedef enum {
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_I8, TYPE_I16, TYPE_I32, TYPE_I64, TYPE_I128,
    TYPE_U8, TYPE_U16, TYPE_U32, TYPE_U64, TYPE_U128,
    TYPE_F32, TYPE_F64, TYPE_F128,
    TYPE_STRING,
    TYPE_ARRAY,
    TYPE_POINTER,
    TYPE_FUNCTION,
    TYPE_INFERRED,
} TypeKind;

// Type representation
typedef struct Type {
    TypeKind kind;
    size_t size;           // Size in bytes
    struct Type *base;     // For arrays and pointers
    struct Type *return_type;  // For functions
    struct Type **param_types; // For functions
    size_t param_count;        // For functions
} Type;

// Built-in types
extern Type g_type_void;
extern Type g_type_bool;
extern Type g_type_i32;
extern Type g_type_i64;
extern Type g_type_f32;
extern Type g_type_f64;
extern Type g_type_string;

//=============================================================================
// AST Node Types (Visitor Pattern)
//=============================================================================

typedef enum {
    // Program and declarations
    AST_PROGRAM,
    AST_IMPORT,
    AST_EXPORT,
    AST_FUNCTION,
    AST_VARIABLE_DECL,
    AST_PARAM_LIST,
    AST_PARAM,
    
    // Statements
    AST_BLOCK,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_FOR_STMT,
    AST_RETURN_STMT,
    AST_EXPR_STMT,
    AST_PRINT_STMT,
    
    // Expressions
    AST_BINARY_EXPR,
    AST_UNARY_EXPR,
    AST_CALL_EXPR,
    AST_INDEX_EXPR,
    AST_MEMBER_EXPR,
    
    // Literals and identifiers
    AST_LITERAL_INT,
    AST_LITERAL_FLOAT,
    AST_LITERAL_STRING,
    AST_LITERAL_BOOL,
    AST_IDENTIFIER,
    
    // Control
    AST_BREAK_STMT,
    AST_CONTINUE_STMT,
} ASTNodeKind;

// Binary operators
typedef enum {
    OP_ADD,             // +
    OP_SUB,             // -
    OP_MUL,             // *
    OP_DIV,             // /
    OP_MOD,             // %
    OP_EQ,              // ==
    OP_NE,              // !=
    OP_LT,              // <
    OP_LE,              // <=
    OP_GT,              // >
    OP_GE,              // >=
    OP_AND,             // &&
    OP_OR,              // ||
    OP_BIT_AND,         // &
    OP_BIT_OR,          // |
    OP_BIT_OR_ASSIGN,   // |=
    OP_BIT_XOR,         // ^
    OP_SHL,             // <<
    OP_SHR,             // >>
    OP_ASSIGN,          // =
    OP_ADD_ASSIGN,      // +=
    OP_SUB_ASSIGN,      // -=
    OP_MUL_ASSIGN,      // *=
    OP_DIV_ASSIGN,      // /=
} BinaryOp;

// Unary operators
typedef enum {
    OP_NEG,         // -x
    OP_NOT,         // !x
    OP_BIT_NOT,     // ~x
    OP_DEREF,       // *x
    OP_ADDR_OF,     // &x
    OP_PRE_INC,     // ++x
    OP_PRE_DEC,     // --x
    OP_POST_INC,    // x++
    OP_POST_DEC,    // x--
} UnaryOp;

// AST Node base structure (for visitor pattern)
struct ASTNode {
    ASTNodeKind kind;
    Type *type;          // Inferred type (set during type checking)
    int line;
    int column;
};

// Forward declare all node types
#define AST_NODE_FIELDS \
    ASTNodeKind kind;    \
    Type *type;          \
    int line;            \
    int column

// Program node - root of the AST
struct ASTProgram {
    AST_NODE_FIELDS;
    ASTNode **exports;
    size_t export_count;
    ASTNode **imports;
    size_t import_count;
    ASTNode **functions;
    size_t function_count;
};

// Import statement
typedef struct ASTImport {
    AST_NODE_FIELDS;
    char *module_name;
} ASTImport;

// Export statement
typedef struct ASTExport {
    AST_NODE_FIELDS;
    ASTNode *target;  // Function or variable to export
} ASTExport;

// Function declaration
typedef struct ASTFunction {
    AST_NODE_FIELDS;
    char *name;
    Type *func_type;     // Function type
    ASTNode **params;    // Parameter list
    size_t param_count;
    ASTNode *body;       // Block statement
    bool is_exported;
    bool is_extern;
} ASTFunction;

// Parameter
typedef struct ASTParam {
    AST_NODE_FIELDS;
    char *name;
    Type *param_type;
} ASTParam;

// Variable declaration
typedef struct ASTVariableDecl {
    AST_NODE_FIELDS;
    char *name;
    Type *var_type;      // Declared type (may be inferred)
    ASTNode *init;       // Initial value (can be NULL for extern)
    bool is_mutable;
} ASTVariableDecl;

// Block statement
typedef struct ASTBlock {
    AST_NODE_FIELDS;
    ASTNode **statements;
    size_t statement_count;
} ASTBlock;

// If statement
typedef struct ASTIfStmt {
    AST_NODE_FIELDS;
    ASTNode *condition;
    ASTNode *then_branch;   // Can be block or single statement
    ASTNode *else_branch;   // Can be NULL, block, or if statement
} ASTIfStmt;

// While loop
typedef struct ASTWhileStmt {
    AST_NODE_FIELDS;
    ASTNode *condition;
    ASTNode *body;
} ASTWhileStmt;

// For loop
typedef struct ASTForStmt {
    AST_NODE_FIELDS;
    ASTNode *init;       // Variable declaration or expression
    ASTNode *condition;  // Can be NULL for infinite loop
    ASTNode *update;     // Can be NULL
    ASTNode *body;
} ASTForStmt;

// Return statement
typedef struct ASTReturnStmt {
    AST_NODE_FIELDS;
    ASTNode *value;      // Can be NULL for void returns
} ASTReturnStmt;

// Expression statement (expression with semicolon)
typedef struct ASTExprStmt {
    AST_NODE_FIELDS;
    ASTNode *expr;
} ASTExprStmt;

// Print statement
typedef struct ASTPrintStmt {
    AST_NODE_FIELDS;
    ASTNode **args;
    size_t arg_count;
    bool is_println;     // println vs print
} ASTPrintStmt;

// Binary expression
typedef struct ASTBinaryExpr {
    AST_NODE_FIELDS;
    BinaryOp op;
    ASTNode *left;
    ASTNode *right;
} ASTBinaryExpr;

// Unary expression
typedef struct ASTUnaryExpr {
    AST_NODE_FIELDS;
    UnaryOp op;
    ASTNode *operand;
} ASTUnaryExpr;

// Function call expression
typedef struct ASTCallExpr {
    AST_NODE_FIELDS;
    ASTNode *callee;     // Function identifier or expression
    ASTNode **args;
    size_t arg_count;
} ASTCallExpr;

// Index expression (array[index])
typedef struct ASTIndexExpr {
    AST_NODE_FIELDS;
    ASTNode *base;
    ASTNode *index;
} ASTIndexExpr;

// Member expression (struct.member)
typedef struct ASTMemberExpr {
    AST_NODE_FIELDS;
    ASTNode *object;
    char *member;
} ASTMemberExpr;

// Literal values
typedef struct ASTLiteralInt {
    AST_NODE_FIELDS;
    int64_t value;
} ASTLiteralInt;

typedef struct ASTLiteralFloat {
    AST_NODE_FIELDS;
    double value;
} ASTLiteralFloat;

typedef struct ASTLiteralString {
    AST_NODE_FIELDS;
    char *value;
} ASTLiteralString;

typedef struct ASTLiteralBool {
    AST_NODE_FIELDS;
    bool value;
} ASTLiteralBool;

// Identifier
typedef struct ASTIdentifier {
    AST_NODE_FIELDS;
    char *name;
} ASTIdentifier;

//=============================================================================
// Visitor Pattern
//=============================================================================

// Visitor function pointer type
typedef void (*ASTVisitorFunc)(ASTNode *node, void *data);

// Visit function for each node type
typedef void (*ASTVisitProgram)(ASTProgram *prog, void *data);
typedef void (*ASTVisitFunction)(ASTFunction *func, void *data);
typedef void (*ASTVisitVariableDecl)(ASTVariableDecl *decl, void *data);
typedef void (*ASTVisitBlock)(ASTBlock *block, void *data);
typedef void (*ASTVisitIfStmt)(ASTIfStmt *if_stmt, void *data);
typedef void (*ASTVisitWhileStmt)(ASTWhileStmt *while_stmt, void *data);
typedef void (*ASTVisitForStmt)(ASTForStmt *for_stmt, void *data);
typedef void (*ASTVisitReturnStmt)(ASTReturnStmt *ret, void *data);
typedef void (*ASTVisitExprStmt)(ASTExprStmt *expr_stmt, void *data);
typedef void (*ASTVisitPrintStmt)(ASTPrintStmt *print, void *data);
typedef void (*ASTVisitBinaryExpr)(ASTBinaryExpr *bin, void *data);
typedef void (*ASTVisitUnaryExpr)(ASTUnaryExpr *unary, void *data);
typedef void (*ASTVisitCallExpr)(ASTCallExpr *call, void *data);
typedef void (*ASTVisitIndexExpr)(ASTIndexExpr *index, void *data);
typedef void (*ASTVisitMemberExpr)(ASTMemberExpr *member, void *data);
typedef void (*ASTVisitLiteralInt)(ASTLiteralInt *lit, void *data);
typedef void (*ASTVisitLiteralFloat)(ASTLiteralFloat *lit, void *data);
typedef void (*ASTVisitLiteralString)(ASTLiteralString *lit, void *data);
typedef void (*ASTVisitLiteralBool)(ASTLiteralBool *lit, void *data);
typedef void (*ASTVisitIdentifier)(ASTIdentifier *ident, void *data);

// Visitor structure
typedef struct ASTVisitor {
    ASTVisitProgram visit_program;
    ASTVisitFunction visit_function;
    ASTVisitVariableDecl visit_variable_decl;
    ASTVisitBlock visit_block;
    ASTVisitIfStmt visit_if_stmt;
    ASTVisitWhileStmt visit_while_stmt;
    ASTVisitForStmt visit_for_stmt;
    ASTVisitReturnStmt visit_return_stmt;
    ASTVisitExprStmt visit_expr_stmt;
    ASTVisitPrintStmt visit_print_stmt;
    ASTVisitBinaryExpr visit_binary_expr;
    ASTVisitUnaryExpr visit_unary_expr;
    ASTVisitCallExpr visit_call_expr;
    ASTVisitIndexExpr visit_index_expr;
    ASTVisitMemberExpr visit_member_expr;
    ASTVisitLiteralInt visit_literal_int;
    ASTVisitLiteralFloat visit_literal_float;
    ASTVisitLiteralString visit_literal_string;
    ASTVisitLiteralBool visit_literal_bool;
    ASTVisitIdentifier visit_identifier;
} ASTVisitor;

// Accept method - implements the visitor pattern
void ast_node_accept(ASTNode *node, ASTVisitor *visitor, void *data);

// Convenience macros for creating nodes (uses arena allocator)
#define ast_new(arena, type) ((type*)arena_alloc(arena, sizeof(type)))

//=============================================================================
// Parser API
//=============================================================================

// Parse source code and return AST program
ASTProgram *ast_parse(Arena *arena, const char *source);

// Free AST (not needed when using arena - just call arena_reset)
void ast_free(ASTProgram *prog);

//=============================================================================
// Debug / Printing
//=============================================================================

// Print AST to stdout
void ast_print(ASTProgram *prog);

// Get string representation of node kind
const char *ast_node_kind_name(ASTNodeKind kind);

// Get string representation of binary operator
const char *ast_binary_op_name(BinaryOp op);

// Get string representation of unary operator
const char *ast_unary_op_name(UnaryOp op);

#endif // EMERALD_AST_H
