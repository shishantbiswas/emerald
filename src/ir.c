#include "ir.h"
#include "ast.h"
#include "hash_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <unistd.h>

//=============================================================================
// Global Types
//=============================================================================

IRType g_ir_type_void = { .kind = IR_TYPE_VOID };
IRType g_ir_type_i32 = { .kind = IR_TYPE_I32 };
IRType g_ir_type_i64 = { .kind = IR_TYPE_I64 };
IRType g_ir_type_f32 = { .kind = IR_TYPE_F32 };
IRType g_ir_type_f64 = { .kind = IR_TYPE_F64 };
IRType g_ir_type_string = { .kind = IR_TYPE_STRING };

//=============================================================================
// Helper Functions
//=============================================================================

static IRModule *current_module;
static IRFunction *current_function;
static IRBasicBlock *current_block;
static Arena *current_arena;
static Hashtable *string_table;
static Hashtable *variable_table;

IRType *ir_type_from_ast(Type *ast_type) {
    if (!ast_type) return &g_ir_type_i32;
    
    switch (ast_type->kind) {
        case TYPE_VOID: return &g_ir_type_void;
        case TYPE_I8: case TYPE_I16: case TYPE_I32: return &g_ir_type_i32;
        case TYPE_I64: case TYPE_I128: return &g_ir_type_i64;
        case TYPE_F32: return &g_ir_type_f32;
        case TYPE_F64: case TYPE_F128: return &g_ir_type_f64;
        case TYPE_STRING: return &g_ir_type_string;
        default: return &g_ir_type_i32;
    }
}

IRValue *ir_const_int(IRType *type, int64_t value) {
    IRValue *val = arena_alloc_type(current_arena, IRValue);
    val->kind = IR_VALUE_CONST;
    val->type = type;
    val->data.const_int = value;
    return val;
}

IRValue *ir_const_float(IRType *type, double value) {
    IRValue *val = arena_alloc_type(current_arena, IRValue);
    val->kind = IR_VALUE_CONST;
    val->type = type;
    val->data.const_float = value;
    return val;
}

IRValue *ir_const_string(IRModule *mod, IRType *type, const char *value) {
    IRGlobal *global = ir_add_string(mod, value);
    IRValue *val = arena_alloc_type(current_arena, IRValue);
    val->kind = IR_VALUE_GLOBAL;
    val->type = type;
    val->id = 0; // Not used for globals
    val->data.global_name = global->name;
    return val;
}

IRValue *ir_arg(IRType *type, size_t index) {
    IRValue *val = arena_alloc_type(current_arena, IRValue);
    val->kind = IR_VALUE_ARG;
    val->type = type;
    val->data.arg_index = index;
    return val;
}

IRValue *ir_temp(IRModule *mod, IRType *type) {
    IRValue *val = arena_alloc_type(current_arena, IRValue);
    val->kind = IR_VALUE_INST;
    val->type = type;
    val->id = mod->temp_counter++;
    val->data.inst = NULL; // Will be set when instruction is created
    return val;
}

// Generate cast if needed
static IRValue *generate_cast(IRValue *value, IRType *target_type) {
    if (value->type == target_type) return value;

    IROpcode opcode = IR_BITCAST; // Default

    // Determine appropriate cast opcode
    if (value->type->kind == IR_TYPE_I32 && target_type->kind == IR_TYPE_F64) {
        opcode = IR_SITOFPD;
    } else if (value->type->kind == IR_TYPE_F64 && target_type->kind == IR_TYPE_I32) {
        opcode = IR_FPTOSI;
    } else if (value->type->kind == IR_TYPE_I64 && target_type->kind == IR_TYPE_F64) {
        opcode = IR_SITOFPD; // Assume signed
    } else if (value->type->kind == IR_TYPE_F64 && target_type->kind == IR_TYPE_I64) {
        opcode = IR_FPTOSI;
    } else if (value->type->kind == IR_TYPE_I8 && (target_type->kind == IR_TYPE_I32 || target_type->kind == IR_TYPE_I64)) {
        opcode = IR_SEXT; // Sign extend
    } else if (value->type->kind == IR_TYPE_I16 && (target_type->kind == IR_TYPE_I32 || target_type->kind == IR_TYPE_I64)) {
        opcode = IR_SEXT;
    } else if (value->type->kind == IR_TYPE_I32 && target_type->kind == IR_TYPE_I64) {
        opcode = IR_SEXT;
    } else {
        // For other cases, use bitcast (e.g., pointer casts, same size types)
        opcode = IR_BITCAST;
    }

    IRInstruction *cast = ir_inst_create(current_block, opcode);
    cast->arg = value;
    cast->type = target_type;
    IRValue *result = ir_temp(current_module, target_type);
    cast->result = result;
    result->data.inst = cast;
    return result;
}

//=============================================================================
// Module Creation
//=============================================================================

IRModule *ir_module_create(Arena *arena, const char *name) {
    IRModule *mod = arena_alloc_type(arena, IRModule);
    mod->name = strdup(name);
    mod->functions = NULL;
    mod->function_count = 0;
    mod->globals = NULL;
    mod->global_count = 0;
    mod->temp_counter = 0;
    mod->global_counter = 0;
    mod->label_counter = 0;
    current_module = mod;
    current_arena = arena;
    return mod;
}

IRFunction *ir_function_create(IRModule *mod, const char *name, IRType *return_type) {
    IRFunction *func = arena_alloc_type(current_arena, IRFunction);
    func->name = strdup(name);
    func->return_type = return_type;
    func->param_types = NULL;
    func->param_count = 0;
    func->is_variadic = false;
    func->is_exported = false;
    func->blocks = NULL;
    func->last_block = NULL;
    func->entry_block = NULL;
    
    mod->functions = realloc(mod->functions, sizeof(IRFunction*) * (mod->function_count + 1));
    mod->functions[mod->function_count++] = func;
    
    current_function = func;
    return func;
}

IRBasicBlock *ir_basic_block_create(IRFunction *func, const char *name) {
    IRBasicBlock *block = arena_alloc_type(current_arena, IRBasicBlock);
    snprintf(block->name, sizeof(block->name), "%s", name);
    block->instructions = NULL;
    block->last_instruction = NULL;
    block->next = NULL;
    
    if (func->blocks == NULL) {
        func->blocks = block;
        func->entry_block = block;
    } else {
        func->last_block->next = block;
    }
    func->last_block = block;
    current_block = block;
    return block;
}

IRInstruction *ir_inst_create(IRBasicBlock *block, IROpcode opcode) {
    IRInstruction *inst = arena_alloc_type(current_arena, IRInstruction);
    inst->opcode = opcode;
    inst->type = &g_ir_type_i32;
    inst->result = NULL;
    inst->arg1 = NULL;
    inst->arg2 = NULL;
    inst->arg = NULL;
    inst->target = NULL;
    inst->true_target = NULL;
    inst->false_target = NULL;
    inst->callee_name = NULL;
    inst->args = NULL;
    inst->arg_count = 0;
    inst->phi_args = NULL;
    inst->phi_arg_count = 0;
    inst->alloc_size = NULL;
    inst->mem_addr = NULL;
    inst->index = NULL;
    inst->element_size = 0;
    inst->next = NULL;
    
    if (block->instructions == NULL) {
        block->instructions = inst;
    } else {
        block->last_instruction->next = inst;
    }
    block->last_instruction = inst;
    return inst;
}

IRGlobal *ir_add_string(IRModule *mod, const char *value) {
    // Check if string already exists
    for (size_t i = 0; i < mod->global_count; i++) {
        if (mod->globals[i]->kind == IR_GLOBAL_STRING &&
            strcmp(mod->globals[i]->string_value, value) == 0) {
            return mod->globals[i];
        }
    }
    
    IRGlobal *global = arena_alloc_type(current_arena, IRGlobal);
    global->kind = IR_GLOBAL_STRING;
    global->id = mod->global_counter++;
    char name[32];
    snprintf(name, sizeof(name), "str%zu", global->id);
    global->name = strdup(name);
    global->type = &g_ir_type_string;
    global->init_value = NULL;
    global->string_value = strdup(value);
    global->next = NULL;
    
    mod->globals = realloc(mod->globals, sizeof(IRGlobal*) * (mod->global_count + 1));
    mod->globals[mod->global_count++] = global;
    
    return global;
}

//=============================================================================
// Code Generation - AST to IR
//=============================================================================

static void generate_statement(ASTNode *node);
static IRValue *generate_expression(ASTNode *node);

// Generate a literal
static IRValue *generate_literal(ASTNode *node) {
    switch (node->kind) {
        case AST_LITERAL_INT: {
            ASTLiteralInt *lit = (ASTLiteralInt*)node;
            return ir_const_int(ir_type_from_ast(node->type), lit->value);
        }
        case AST_LITERAL_FLOAT: {
            ASTLiteralFloat *lit = (ASTLiteralFloat*)node;
            return ir_const_float(ir_type_from_ast(node->type), lit->value);
        }
        case AST_LITERAL_STRING: {
            ASTLiteralString *lit = (ASTLiteralString*)node;
            return ir_const_string(current_module, &g_ir_type_string, lit->value);
        }
        case AST_LITERAL_BOOL: {
            ASTLiteralBool *lit = (ASTLiteralBool*)node;
            return ir_const_int(&g_ir_type_i32, lit->value ? 1 : 0);
        }
        default:
            return NULL;
    }
}

// Generate identifier (variable lookup)
static IRValue *generate_identifier(ASTNode *node) {
    ASTIdentifier *ident = (ASTIdentifier*)node;
    IRValue *addr = (IRValue*)findEntry(variable_table, ident->name);
    if (addr) {
        IRInstruction *load = ir_inst_create(current_block, IR_LOAD);
        load->mem_addr = addr;
        load->type = addr->type;
        IRValue *result = ir_temp(current_module, load->type);
        load->result = result;
        result->data.inst = load;
        return result;
    } else {
        // Function or undefined, for now temp
        return ir_temp(current_module, &g_ir_type_i64);
    }
}

// Get QBE instruction suffix for type
static const char *get_type_suffix(IRType *type) {
    switch (type->kind) {
        case IR_TYPE_I8:
        case IR_TYPE_I16:
        case IR_TYPE_I32:
            return "w";
        case IR_TYPE_I64:
        case IR_TYPE_I128:
        case IR_TYPE_PTR:
            return "l";
        case IR_TYPE_F32:
            return "s";
        case IR_TYPE_F64:
            return "d";
        default:
            return "w";
    }
}

// Get QBE comparison suffix
static const char *get_cmp_suffix(IRCmpKind kind) {
    switch (kind) {
        case IR_CMP_EQ: return "eq";
        case IR_CMP_NE: return "ne";
        case IR_CMP_ULT: return "ult";
        case IR_CMP_ULE: return "ule";
        case IR_CMP_UGT: return "ugt";
        case IR_CMP_UGE: return "uge";
        case IR_CMP_SLT: return "lt";
        case IR_CMP_SLE: return "le";
        case IR_CMP_SGT: return "gt";
        case IR_CMP_SGE: return "ge";
        default: return "eq";
    }
}

// Get size of IR type in bytes
static size_t ir_type_size(IRType *type) {
    switch (type->kind) {
        case IR_TYPE_I8: return 1;
        case IR_TYPE_I16: return 2;
        case IR_TYPE_I32: return 4;
        case IR_TYPE_I64:
        case IR_TYPE_I128:
        case IR_TYPE_PTR: return 8;
        case IR_TYPE_F32: return 4;
        case IR_TYPE_F64: return 8;
        default: return 8;
    }
}

// Generate binary expression
static IRValue *generate_binary_expr(ASTNode *node) {
    ASTBinaryExpr *bin = (ASTBinaryExpr*)node;

    // Handle assignment separately
    if (bin->op == OP_ASSIGN) {
        // Assume left is identifier
        if (bin->left->kind == AST_IDENTIFIER) {
            ASTIdentifier *ident = (ASTIdentifier*)bin->left;
            IRValue *addr = (IRValue*)findEntry(variable_table, ident->name);
            if (addr) {
                IRValue *value = generate_expression(bin->right);
                value = generate_cast(value, addr->type); // Cast to variable type
                IRInstruction *store = ir_inst_create(current_block, IR_STORE);
                store->mem_addr = addr;
                store->arg = value;
                return value; // Assignment returns the value
            }
        }
        // Error case
        return ir_const_int(&g_ir_type_i32, 0);
    }

    IRValue *left = generate_expression(bin->left);
    IRValue *right = generate_expression(bin->right);

    // Type coercion: promote to common type
    IRType *result_type = left->type;
    if (left->type->kind == IR_TYPE_F64 || right->type->kind == IR_TYPE_F64) {
        result_type = &g_ir_type_f64;
    } else if (left->type->kind == IR_TYPE_F32 || right->type->kind == IR_TYPE_F32) {
        result_type = &g_ir_type_f32;
    } else if (left->type->kind == IR_TYPE_I64 || right->type->kind == IR_TYPE_I64) {
        result_type = &g_ir_type_i64;
    }

    // Cast operands if needed
    left = generate_cast(left, result_type);
    right = generate_cast(right, result_type);

    IRInstruction *inst = ir_inst_create(current_block, IR_ADD);
    inst->arg1 = left;
    inst->arg2 = right;
    inst->type = result_type;

    // Create result value
    IRValue *result = ir_temp(current_module, result_type);
    inst->result = result;
    result->data.inst = inst;

    // Set opcode based on operator
    switch (bin->op) {
        case OP_ADD: inst->opcode = IR_ADD; break;
        case OP_SUB: inst->opcode = IR_SUB; break;
        case OP_MUL: inst->opcode = IR_MUL; break;
        case OP_DIV: inst->opcode = IR_DIV; break;
        case OP_MOD: inst->opcode = IR_MOD; break;
        case OP_EQ: case OP_NE: case OP_LT: case OP_LE:
        case OP_GT: case OP_GE:
            inst->opcode = IR_CMP;
            result_type = &g_ir_type_i32;
            inst->type = result_type;
            // Set comparison kind
            switch (bin->op) {
                case OP_EQ: inst->cmp_kind = IR_CMP_EQ; break;
                case OP_NE: inst->cmp_kind = IR_CMP_NE; break;
                case OP_LT: inst->cmp_kind = IR_CMP_SLT; break;
                case OP_LE: inst->cmp_kind = IR_CMP_SLE; break;
                case OP_GT: inst->cmp_kind = IR_CMP_SGT; break;
                case OP_GE: inst->cmp_kind = IR_CMP_SGE; break;
                default: inst->cmp_kind = IR_CMP_EQ; break;
            }
            break;
        case OP_AND: inst->opcode = IR_AND; break;
        case OP_OR: inst->opcode = IR_OR; break;
        default:
            inst->opcode = IR_ADD;
    }

    return result;
}

// Generate unary expression
static IRValue *generate_unary_expr(ASTNode *node) {
    ASTUnaryExpr *unary = (ASTUnaryExpr*)node;
    IRValue *operand = generate_expression(unary->operand);
    
    IRInstruction *inst = ir_inst_create(current_block, IR_SUB);
    inst->arg = operand;
    inst->type = operand->type;
    
    IRValue *result = ir_temp(current_module, inst->type);
    inst->result = result;
    result->data.inst = inst;
    
    switch (unary->op) {
        case OP_NEG: inst->opcode = IR_SUB; break;
        case OP_NOT: inst->opcode = IR_XOR; break;
        case OP_BIT_NOT: inst->opcode = IR_XOR; break;
        case OP_DEREF: inst->opcode = IR_LOAD; break;
        case OP_ADDR_OF: inst->opcode = IR_ALLOC; break;
        default: inst->opcode = IR_SUB;
    }
    
    return result;
}

// Generate function call
static IRValue *generate_call(ASTNode *node) {
    ASTCallExpr *call = (ASTCallExpr*)node;
    
    // Generate arguments
    IRValue **args = arena_alloc_array(current_arena, IRValue*, call->arg_count);
    for (size_t i = 0; i < call->arg_count; i++) {
        args[i] = generate_expression(call->args[i]);
    }
    
    IRInstruction *inst = ir_inst_create(current_block, IR_CALL);
    inst->args = args;
    inst->arg_count = call->arg_count;
    
    // Get callee name
    if (call->callee->kind == AST_IDENTIFIER) {
        ASTIdentifier *ident = (ASTIdentifier*)call->callee;
        inst->callee_name = strdup(ident->name);
    }
    
    inst->type = &g_ir_type_i64;
    IRValue *result = ir_temp(current_module, inst->type);
    inst->result = result;
    result->data.inst = inst;
    
    return result;
}

// Generate expression
static IRValue *generate_expression(ASTNode *node) {
    if (!node) return NULL;
    
    switch (node->kind) {
        case AST_LITERAL_INT:
        case AST_LITERAL_FLOAT:
        case AST_LITERAL_STRING:
        case AST_LITERAL_BOOL:
            return generate_literal(node);
            
        case AST_IDENTIFIER:
            return generate_identifier(node);
            
        case AST_BINARY_EXPR:
            return generate_binary_expr(node);
            
        case AST_UNARY_EXPR:
            return generate_unary_expr(node);
            
        case AST_CALL_EXPR:
            return generate_call(node);
            
        default:
            return NULL;
    }
}

// Generate block
static void generate_block(ASTNode *node) {
    ASTBlock *block = (ASTBlock*)node;
    for (size_t i = 0; i < block->statement_count; i++) {
        generate_statement(block->statements[i]);
    }
}

// Generate if statement
static void generate_if(ASTNode *node) {
    ASTIfStmt *if_stmt = (ASTIfStmt*)node;
    
    // Generate condition
    IRValue *cond = generate_expression(if_stmt->condition);
    
    // Create blocks
    IRBasicBlock *then_block = ir_basic_block_create(current_function, "if.then");
    IRBasicBlock *else_block = if_stmt->else_branch ? 
        ir_basic_block_create(current_function, "if.else") : NULL;
    IRBasicBlock *merge_block = ir_basic_block_create(current_function, "if.end");
    
    // Conditional branch
    IRInstruction *cbr = ir_inst_create(current_block, IR_CBR);
    cbr->arg = cond;
    cbr->true_target = then_block;
    cbr->false_target = else_block ? else_block : merge_block;
    
    // Then block
    current_block = then_block;
    generate_statement(if_stmt->then_branch);
    IRInstruction *br_then = ir_inst_create(current_block, IR_BR);
    br_then->target = merge_block;
    
    // Else block
    if (else_block) {
        current_block = else_block;
        generate_statement(if_stmt->else_branch);
        IRInstruction *br_else = ir_inst_create(current_block, IR_BR);
        br_else->target = merge_block;
    }
    
    // Merge block
    current_block = merge_block;
}


// Generate for loop
static void generate_for(ASTNode *node) {
    ASTForStmt *for_stmt = (ASTForStmt*)node;
    
    IRBasicBlock *init_block = ir_basic_block_create(current_function, "for.init");
    IRBasicBlock *cond_block = ir_basic_block_create(current_function, "for.cond");
    IRBasicBlock *update_block = ir_basic_block_create(current_function, "for.update");
    IRBasicBlock *body_block = ir_basic_block_create(current_function, "for.body");
    IRBasicBlock *end_block = ir_basic_block_create(current_function, "for.end");
    
    // Init block
    current_block = init_block;
    if (for_stmt->init) {
        generate_statement(for_stmt->init);
    }
    IRInstruction *br_init = ir_inst_create(current_block, IR_BR);
    br_init->target = cond_block;
    
    // Condition block
    current_block = cond_block;
    if (for_stmt->condition) {
        IRValue *cond = generate_expression(for_stmt->condition);
        IRInstruction *cbr = ir_inst_create(current_block, IR_CBR);
        cbr->arg = cond;
        cbr->true_target = body_block;
        cbr->false_target = end_block;
    } else {
        // Infinite loop
        IRInstruction *br = ir_inst_create(current_block, IR_BR);
        br->target = body_block;
    }
    
    // Update block (jump back to condition)
    current_block = update_block;
    if (for_stmt->update) {
        generate_expression(for_stmt->update); // Expression statement
    }
    IRInstruction *br_update = ir_inst_create(current_block, IR_BR);
    br_update->target = cond_block;
    
    // Body block
    current_block = body_block;
    generate_statement(for_stmt->body);
    IRInstruction *br_body = ir_inst_create(current_block, IR_BR);
    br_body->target = update_block;
    
    // End block
    current_block = end_block;
}

// Generate return statement
static void generate_return(ASTNode *node) {
    ASTReturnStmt *ret = (ASTReturnStmt*)node;
    
    IRValue *ret_val = NULL;
    if (ret->value) {
        ret_val = generate_expression(ret->value);
    }
    
    IRInstruction *inst = ir_inst_create(current_block, IR_RET);
    inst->arg = ret_val;
}

// Generate print statement
static void generate_print(ASTNode *node) {
    ASTPrintStmt *print = (ASTPrintStmt*)node;
    
    // Handle each argument
    for (size_t i = 0; i < print->arg_count; i++) {
        ASTNode *arg = print->args[i];
        
        if (arg->kind == AST_LITERAL_STRING) {
            // Print string
            ASTLiteralString *lit = (ASTLiteralString*)arg;

            IRInstruction *call = ir_inst_create(current_block, IR_CALL);
            call->callee_name = strdup("puts");
            call->args = arena_alloc_array(current_arena, IRValue*, 1);
            call->args[0] = ir_const_string(current_module, &g_ir_type_string, lit->value);
            call->arg_count = 1;
            call->type = &g_ir_type_i32;
            IRValue *result = ir_temp(current_module, call->type);
            call->result = result;
            result->data.inst = call;
        } else {
            // Print integer (use printf with format)
            IRValue *val = generate_expression(arg);

            const char *format = (val->type->kind == IR_TYPE_I64) ? "%lld" : "%d";

            IRInstruction *call = ir_inst_create(current_block, IR_CALL);
            call->callee_name = strdup("printf");
            call->args = arena_alloc_array(current_arena, IRValue*, 2);
            call->args[0] = ir_const_string(current_module, &g_ir_type_string, format);
            call->args[1] = val;
            call->arg_count = 2;
            call->type = &g_ir_type_i32;
            IRValue *result = ir_temp(current_module, call->type);
            call->result = result;
            result->data.inst = call;
        }
        
        // Add newline for println
        if (print->is_println) {
            IRInstruction *call = ir_inst_create(current_block, IR_CALL);
            call->callee_name = strdup("puts");
            call->args = arena_alloc_array(current_arena, IRValue*, 1);
            call->args[0] = ir_const_string(current_module, &g_ir_type_string, "");
            call->arg_count = 1;
            call->type = &g_ir_type_i32;
            IRValue *result = ir_temp(current_module, call->type);
            call->result = result;
            result->data.inst = call;
        }
    }
}

// Generate variable declaration
static void generate_variable_decl(ASTNode *node) {
    ASTVariableDecl *decl = (ASTVariableDecl*)node;

    // Allocate space for variable
    IRInstruction *alloc = ir_inst_create(current_block, IR_ALLOC);
    alloc->type = ir_type_from_ast(decl->var_type);

    IRValue *result = ir_temp(current_module, alloc->type);
    alloc->result = result;
    result->data.inst = alloc;

    // Store in variable table
    insertEntry(variable_table, decl->name, (void*)result, 0);

    // Initialize if there's an initializer
    if (decl->init) {
        IRValue *init_val = generate_expression(decl->init);

        IRInstruction *store = ir_inst_create(current_block, IR_STORE);
        store->mem_addr = result;
        store->arg = init_val;
    }
}

// Generate statement
static void generate_statement(ASTNode *node) {
    if (!node) return;
    
    switch (node->kind) {
        case AST_BLOCK:
            generate_block(node);
            break;
            
        case AST_IF_STMT:
            generate_if(node);
            break;

        case AST_FOR_STMT:
            generate_for(node);
            break;
            
        case AST_RETURN_STMT:
            generate_return(node);
            break;
            
        case AST_PRINT_STMT:
            generate_print(node);
            break;
            
        case AST_VARIABLE_DECL:
            generate_variable_decl(node);
            break;
            
        case AST_EXPR_STMT: {
            ASTExprStmt *stmt = (ASTExprStmt*)node;
            generate_expression(stmt->expr);
            break;
        }
        
        default:
            break;
    }
}

// Generate function
static void generate_function(ASTNode *node) {
    ASTFunction *func = (ASTFunction*)node;

    // Create IR function
    IRFunction *ir_func = ir_function_create(current_module, func->name, ir_type_from_ast(func->func_type->return_type));
    ir_func->is_exported = func->is_exported;
    
    // Create entry block
    IRBasicBlock *entry = ir_basic_block_create(ir_func, "entry");
    current_block = entry;
    
    // Generate function body
    if (func->body) {
        generate_block((ASTNode*)func->body);
    }

    // Add implicit return if no explicit return
    if (current_block) {
        // Check if last instruction is a return
        bool has_return = false;
        if (current_block->last_instruction &&
            (current_block->last_instruction->opcode == IR_RET ||
             current_block->last_instruction->opcode == IR_CBR)) {
            has_return = true;
        }
        if (!has_return) {
            IRInstruction *ret = ir_inst_create(current_block, IR_RET);
            ret->arg = ir_const_int(&g_ir_type_i64, 0);
        }
    }
}

// Generate module
static void generate_module(ASTNode *node) {
    ASTProgram *prog = (ASTProgram*)node;
    
    for (size_t i = 0; i < prog->function_count; i++) {
        generate_function(prog->functions[i]);
    }
}

//=============================================================================
// Public API
//=============================================================================

int ir_generate(IRModule *mod, ASTProgram *ast) {
    current_module = mod;
    string_table = createHashtable(128);
    variable_table = createHashtable(128);

    generate_module((ASTNode*)ast);

    freeHashtable(string_table);
    freeHashtable(variable_table);
    return 0;
}

//=============================================================================
// QBE Emitter
//=============================================================================

static void emit_type(FILE *fp, IRType *type);
static void emit_value(FILE *fp, IRValue *val);
static void emit_instruction(FILE *fp, IRInstruction *inst);

// Emit type
static void emit_type(FILE *fp, IRType *type) {
    switch (type->kind) {
        case IR_TYPE_VOID: fprintf(fp, "v"); break;
        case IR_TYPE_I8:
        case IR_TYPE_I16:
        case IR_TYPE_I32: fprintf(fp, "w"); break;
        case IR_TYPE_I64:
        case IR_TYPE_I128:
        case IR_TYPE_PTR: fprintf(fp, "l"); break;
        case IR_TYPE_F32: fprintf(fp, "s"); break;
        case IR_TYPE_F64: fprintf(fp, "d"); break;
        case IR_TYPE_STRING: fprintf(fp, "l"); break;
        default: fprintf(fp, "w"); break;
    }
}

// Emit value
static void emit_value(FILE *fp, IRValue *val) {
    if (!val) {
        fprintf(fp, "null");
        return;
    }
    
    switch (val->kind) {
        case IR_VALUE_CONST:
            if (val->type->kind == IR_TYPE_F64 || val->type->kind == IR_TYPE_F32) {
                fprintf(fp, "%f", val->data.const_float);
            } else {
                fprintf(fp, "%" PRId64, val->data.const_int);
            }
            break;
            
        case IR_VALUE_ARG:
            fprintf(fp, "%%arg%zu", val->data.arg_index);
            break;
            
        case IR_VALUE_INST:
            fprintf(fp, "%%t%zu", val->id);
            break;
            
        case IR_VALUE_GLOBAL:
            fprintf(fp, "$%s", val->data.global_name);
            break;
    }
}

// Get QBE opcode string
static const char *get_qbe_op(IROpcode op) {
    switch (op) {
        case IR_ADD: return "add";
        case IR_SUB: return "sub";
        case IR_MUL: return "mul";
        case IR_DIV: return "div";
        case IR_MOD: return "rem";
        case IR_AND: return "and";
        case IR_OR: return "or";
        case IR_XOR: return "xor";
        case IR_SHL: return "shl";
        case IR_SHR: return "shr";
        case IR_CMP: return "cmp";
        default: return "add";
    }
}

// Emit instruction
static void emit_instruction(FILE *fp, IRInstruction *inst) {
    if (!inst) return;
    
    switch (inst->opcode) {
        case IR_ADD:
        case IR_SUB:
        case IR_MUL:
        case IR_DIV:
        case IR_MOD:
        case IR_AND:
        case IR_OR:
        case IR_XOR:
        case IR_SHL:
        case IR_SHR: {
            const char *suffix = get_type_suffix(inst->type);
            fprintf(fp, "    %%t%zu =%s %s ", inst->result->id, suffix, get_qbe_op(inst->opcode));
            emit_value(fp, inst->arg1);
            fprintf(fp, ", ");
            emit_value(fp, inst->arg2);
            fprintf(fp, "\n");
            break;
        }
        
        case IR_CMP: {
            fprintf(fp, "    %%t%zu =%s c%s%s ", inst->result->id, get_type_suffix(inst->type), get_cmp_suffix(inst->cmp_kind), get_type_suffix(inst->arg1->type));
            emit_value(fp, inst->arg1);
            fprintf(fp, ", ");
            emit_value(fp, inst->arg2);
            fprintf(fp, "\n");
            break;
        }
        
        case IR_RET:
            if (inst->arg) {
                fprintf(fp, "    ret ");
                emit_value(fp, inst->arg);
                fprintf(fp, "\n");
            } else {
                fprintf(fp, "    ret\n");
            }
            break;
            
        case IR_BR:
            fprintf(fp, "    jmp @%s\n", inst->target->name);
            break;
            
        case IR_CBR:
            fprintf(fp, "    jnz ");
            emit_value(fp, inst->arg);
            fprintf(fp, ", @%s, @%s\n", 
                inst->true_target->name, 
                inst->false_target->name);
            break;
            
        case IR_CALL: {
            const char *suffix = get_type_suffix(inst->type);
            fprintf(fp, "    %%t%zu =%s call $%s(", inst->result->id, suffix, inst->callee_name);
            for (size_t i = 0; i < inst->arg_count; i++) {
                if (i > 0) fprintf(fp, ", ");
                emit_type(fp, inst->args[i]->type);
                fprintf(fp, " ");
                emit_value(fp, inst->args[i]);
            }
            fprintf(fp, ")\n");
            break;
        }
        
        case IR_ALLOC: {
            fprintf(fp, "    %%t%zu =l alloc %zu\n", inst->result->id, ir_type_size(inst->type));
            break;
        }
        
        case IR_LOAD: {
            fprintf(fp, "    %%t%zu =l load ", inst->result->id);
            emit_value(fp, inst->mem_addr);
            fprintf(fp, "\n");
            break;
        }
        
        case IR_STORE: {
            fprintf(fp, "    store ");
            emit_value(fp, inst->arg);
            fprintf(fp, ", ");
            emit_value(fp, inst->mem_addr);
            fprintf(fp, "\n");
            break;
        }

        case IR_BITCAST: {
            fprintf(fp, "    %%t%zu =%s cast %%t%zu\n", inst->result->id, get_type_suffix(inst->type), inst->arg->id);
            break;
        }

        case IR_SEXT: {
            fprintf(fp, "    %%t%zu =%s cast %%t%zu\n", inst->result->id, get_type_suffix(inst->type), inst->arg->id);
            break;
        }

        case IR_SITOFPD: {
            fprintf(fp, "    %%t%zu =d extsw %%t%zu\n", inst->result->id, inst->arg->id);
            break;
        }

        case IR_FPTOSI: {
            fprintf(fp, "    %%t%zu =w truncd %%t%zu\n", inst->result->id, inst->arg->id);
            break;
        }

        default:
            break;
    }
}

// Emit basic block
static void emit_basic_block(FILE *fp, IRBasicBlock *block) {
    fprintf(fp, "@%s\n", block->name);
    
    IRInstruction *inst = block->instructions;
    while (inst) {
        emit_instruction(fp, inst);
        inst = inst->next;
    }
}

// Emit function
static void emit_function(FILE *fp, IRFunction *func) {
    // Function signature
    fprintf(fp, "export function ");
    emit_type(fp, func->return_type);
    fprintf(fp, " $%s(", func->name);
    
    // Parameters
    for (size_t i = 0; i < func->param_count; i++) {
        if (i > 0) fprintf(fp, ", ");
        emit_type(fp, func->param_types[i]);
        fprintf(fp, " %%arg%zu", i);
    }
    fprintf(fp, ") {\n");
    
    // Basic blocks
    IRBasicBlock *block = func->blocks;
    while (block) {
        emit_basic_block(fp, block);
        block = block->next;
    }
    
    fprintf(fp, "}\n\n");
}

// Emit global
static void emit_global(FILE *fp, IRGlobal *global) {
    if (global->kind == IR_GLOBAL_STRING) {
        fprintf(fp, "data $%s = { b \"%s\", b 0 }\n",
            global->name,
            global->string_value);
    }
}

void ir_print(IRModule *mod, FILE *fp) {
    // Emit string constants first
    for (size_t i = 0; i < mod->global_count; i++) {
        emit_global(fp, mod->globals[i]);
    }
    fprintf(fp, "\n");
    
    // Emit format string for integers
    fprintf(fp, "data $fmt_d = { b \"%%d\\n\", b 0 }\n\n");
    
    // Emit functions
    for (size_t i = 0; i < mod->function_count; i++) {
        emit_function(fp, mod->functions[i]);
    }
}

int ir_emit(IRModule *mod, const char *filename) {
    // Create build directory if needed
    struct stat st = {0};
    if (stat("build", &st) != 0) {
        mkdir("build", 0700);
    }
    
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        return -1;
    }
    
    ir_print(mod, fp);
    fclose(fp);
    
    return 0;
}

void ir_module_free(IRModule *mod) {
    // Arena handles cleanup
    (void)mod;
}
