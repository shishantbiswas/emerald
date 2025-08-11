#include "ast.h"

// Basic AST node creation function
AST_Node* create_ast_node(AST_Node_Type type) {
    AST_Node* node = malloc(sizeof(AST_Node));
    if (!node) return NULL;
    
    node->type = type;
    node->left = NULL;
    node->right = NULL;
    node->value = NULL;
    
    return node;
}

// Function to free an AST
void free_ast(AST_Node* node) {
    if (!node) return;
    
    free_ast(node->left);
    free_ast(node->right);
    
    if (node->value) {
        free(node->value);
    }
    
    free(node);
}