#ifndef EMERALD_IR_H
#define EMERALD_IR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "ast.h"
#include "arena.h"

//=============================================================================
// IR Definitions
//=============================================================================

// QBE type kinds
typedef enum {
    IR_TYPE_VOID,
    IR_TYPE_I8,
    IR_TYPE_I16,
    IR_TYPE_I32,
    IR_TYPE_I64,
    IR_TYPE_I128,
    IR_TYPE_F32,
    IR_TYPE_F64,
    IR_TYPE_STRING,
    IR_TYPE_PTR,
} IRTypeKind;

// IR Type
typedef struct IRType {
    IRTypeKind kind;
    struct IRType *base;  // For pointers
} IRType;

extern IRType g_ir_type_void;
extern IRType g_ir_type_i32;
extern IRType g_ir_type_i64;
extern IRType g_ir_type_f32;
extern IRType g_ir_type_f64;
extern IRType g_ir_type_string;

// Get IR type from AST type
IRType *ir_type_from_ast(Type *ast_type);

//=============================================================================
// IR Values
//=============================================================================

typedef enum {
    IR_VALUE_INST,
    IR_VALUE_CONST,
    IR_VALUE_ARG,
    IR_VALUE_GLOBAL,
} IRValueKind;

// IR Value (used in instructions)
typedef struct IRValue {
    IRValueKind kind;
    IRType *type;
    size_t id;  // Unique ID for temp names
    union {
        struct IRInstruction *inst;
        int64_t const_int;
        double const_float;
        char *const_string;
        size_t arg_index;
        char *global_name;
    } data;
} IRValue;

//=============================================================================
// IR Instructions
//=============================================================================

typedef enum {
    // Memory
    IR_ALLOC,
    IR_LOAD,
    IR_STORE,
    IR_GETPTR,
    
    // Arithmetic
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_MOD,
    
    // Comparison
    IR_CMP,
    
    // Logical
    IR_AND,
    IR_OR,
    IR_XOR,
    
    // Bitwise
    IR_SHL,
    IR_SHR,
    
    // Cast
    IR_ZEXT,    // Zero extend
    IR_SEXT,    // Sign extend
    IR_TRUNC,   // Truncate
    IR_BITCAST, // Bit cast
    IR_SITOFPD, // Signed int to float
    IR_FPTOSI,  // Float to signed int
    
    // Control flow
    IR_BR,
    IR_CBR,
    IR_RET,
    IR_CALL,
    IR_PHI,
    
    // Other
    IR_NOP,
    IR_COPY,
} IROpcode;

// Comparison types
typedef enum {
    IR_CMP_EQ,
    IR_CMP_NE,
    IR_CMP_ULT,  // Unsigned less than
    IR_CMP_ULE,  // Unsigned less or equal
    IR_CMP_UGT,  // Unsigned greater than
    IR_CMP_UGE,  // Unsigned greater or equal
    IR_CMP_SLT,  // Signed less than
    IR_CMP_SLE,  // Signed less or equal
    IR_CMP_SGT,  // Signed greater than
    IR_CMP_SGE,  // Signed greater or equal
} IRCmpKind;

// IR Instruction
typedef struct IRInstruction {
    IROpcode opcode;
    IRType *type;
    IRValue *result;       // Result value (can be NULL for void)
    
    // For binary operations
    IRValue *arg1;
    IRValue *arg2;
    
    // For unary operations
    IRValue *arg;
    
    // For comparisons
    IRCmpKind cmp_kind;
    
    // For branch/call targets
    struct IRBasicBlock *target;
    struct IRBasicBlock *true_target;
    struct IRBasicBlock *false_target;
    
    // For calls
    char *callee_name;
    IRValue **args;
    size_t arg_count;
    
    // For phi
    struct IRPhiArg {
        IRValue *value;
        struct IRBasicBlock *block;
    } *phi_args;
    size_t phi_arg_count;
    
    // For alloc
    IRValue *alloc_size;
    
    // For load/store
    IRValue *mem_addr;
    
    // For getptr
    IRValue *index;
    size_t element_size;
    
    // Linked list
    struct IRInstruction *next;
} IRInstruction;

//=============================================================================
// Basic Blocks
//=============================================================================

typedef struct IRBasicBlock {
    char name[64];
    IRInstruction *instructions;
    IRInstruction *last_instruction;
    struct IRBasicBlock *next;
} IRBasicBlock;

//=============================================================================
// Functions
//=============================================================================

typedef struct IRFunction {
    char *name;
    IRType *return_type;
    IRType **param_types;
    size_t param_count;
    bool is_variadic;
    bool is_exported;
    IRBasicBlock *blocks;
    IRBasicBlock *last_block;
    IRBasicBlock *entry_block;
} IRFunction;

//=============================================================================
// Globals
//=============================================================================

typedef enum {
    IR_GLOBAL_VAR,
    IR_GLOBAL_STRING,
} IRGlobalKind;

typedef struct IRGlobal {
    IRGlobalKind kind;
    size_t id;  // Unique ID for global names
    char *name;
    IRType *type;
    IRValue *init_value;
    char *string_value;  // For string constants
    struct IRGlobal *next;
} IRGlobal;

//=============================================================================
// Module
//=============================================================================

typedef struct IRModule {
    char *name;
    IRFunction **functions;
    size_t function_count;
    IRGlobal **globals;
    size_t global_count;
    size_t temp_counter;  // For generating unique temp names
    size_t global_counter; // For generating unique global names
    size_t label_counter; // For generating unique labels
} IRModule;

//=============================================================================
// Builder API
//=============================================================================

// Create a new module
IRModule *ir_module_create(Arena *arena, const char *name);

// Add a function to the module
IRFunction *ir_function_create(IRModule *mod, const char *name, IRType *return_type);

// Add a basic block to a function
IRBasicBlock *ir_basic_block_create(IRFunction *func, const char *name);

// Add an instruction to a basic block
IRInstruction *ir_inst_create(IRBasicBlock *block, IROpcode opcode);

// Helper functions for creating instructions
IRValue *ir_const_int(IRType *type, int64_t value);
IRValue *ir_const_float(IRType *type, double value);
IRValue *ir_const_string(IRModule *mod, IRType *type, const char *value);
IRValue *ir_arg(IRType *type, size_t index);
IRValue *ir_temp(IRModule *mod, IRType *type);

// Add global string constant
IRGlobal *ir_add_string(IRModule *mod, const char *value);

//=============================================================================
// Code Generation
//=============================================================================

// Generate QBE IR from AST
int ir_generate(IRModule *mod, ASTProgram *ast);

// Emit QBE IR to file
int ir_emit(IRModule *mod, const char *filename);

// Print QBE IR to file (FILE*)
void ir_print(IRModule *mod, FILE *fp);

//=============================================================================
// Cleanup
//=============================================================================

void ir_module_free(IRModule *mod);

#endif // EMERALD_IR_H
