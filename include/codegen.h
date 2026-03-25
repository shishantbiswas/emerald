#ifndef EMERALD_CODEGEN_H
#define EMERALD_CODEGEN_H

#include <stdbool.h>

//=============================================================================
// Code Generation API
//=============================================================================

// Compile QBE SSA file to object file
int codegen_compile_qbe(const char *ssa_file, const char *obj_file);

// Link object file to create final executable
int codegen_link(const char *obj_file, const char *output_file);

// Full pipeline: QBE SSA -> executable
int codegen_build_executable(const char *ssa_file, const char *output_file);

// Check if QBE is available on the system
bool codegen_check_qbe_available(void);

// Get QBE version (for debugging)
const char *codegen_get_qbe_version(void);

#endif // EMERALD_CODEGEN_H
