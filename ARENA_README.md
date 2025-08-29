# Arena Allocator

This directory contains an efficient arena-based memory management system for the Emerald compiler project.

## Overview

The arena allocator provides fast memory allocation and deallocation by managing memory in large chunks (blocks). This is particularly useful for compiler implementations where many small allocations are made during parsing and compilation phases.

## Features

- **Fast allocation**: O(1) allocation time for most cases
- **Efficient deallocation**: Bulk deallocation via `arena_reset()` or `arena_destroy()`
- **Memory alignment**: Support for aligned allocations
- **Flexible block sizes**: Configurable block sizes with sensible defaults
- **Memory tracking**: Built-in memory usage statistics
- **Type-safe macros**: Convenient macros for common allocation patterns

## API Reference

### Core Functions

```c
// Create a new arena allocator
Arena* arena_create(size_t block_size);

// Allocate memory from the arena
void* arena_alloc(Arena* arena, size_t size);

// Allocate aligned memory from the arena
void* arena_alloc_aligned(Arena* arena, size_t size, size_t alignment);

// Reset the arena (free all blocks but keep the arena structure)
void arena_reset(Arena* arena);

// Destroy the arena and free all memory
void arena_destroy(Arena* arena);
```

### Convenience Macros

```c
// Allocate a single instance of a type
#define arena_alloc_type(arena, type) ((type*)arena_alloc(arena, sizeof(type)))

// Allocate an array of a type
#define arena_alloc_array(arena, type, count) ((type*)arena_alloc(arena, sizeof(type) * (count)))
```

### Utility Functions

```c
// Get the total used memory across all blocks
size_t arena_get_used_memory(Arena* arena);

// Get the total allocated memory across all blocks
size_t arena_get_total_memory(Arena* arena);
```

## Usage Examples

### Basic Usage

```c
#include "arena.h"

int main() {
    // Create an arena with 64KB blocks
    Arena* arena = arena_create(64 * 1024);
    
    // Allocate some data
    int* numbers = arena_alloc_array(arena, int, 100);
    char* str = arena_alloc(arena, 50);
    MyStruct* obj = arena_alloc_type(arena, MyStruct);
    
    // Use the allocated memory...
    
    // Clean up
    arena_destroy(arena);
    return 0;
}
```

### Integration with Existing Data Structures

The arena allocator can be easily integrated with existing data structures like the hash table:

```c
// Create hash table using arena
Hashtable* ht = arena_alloc_type(arena, Hashtable);
ht->size = 16;
ht->table = arena_alloc_array(arena, Entry*, ht->size);

// Insert entries using arena
Entry* entry = arena_alloc_type(arena, Entry);
entry->data = arena_alloc(arena, strlen(key) + 1);
strcpy(entry->data, key);
```

### Memory Management Patterns

#### Phase-based Allocation

```c
// Phase 1: Lexical analysis
Arena* lex_arena = arena_create(32 * 1024);
// ... allocate tokens and lexemes ...

// Phase 2: Parsing
Arena* parse_arena = arena_create(64 * 1024);
// ... allocate AST nodes ...

// Phase 3: Code generation
Arena* codegen_arena = arena_create(128 * 1024);
// ... allocate IR nodes ...

// Clean up each phase when done
arena_destroy(lex_arena);
arena_destroy(parse_arena);
arena_destroy(codegen_arena);
```

#### Temporary Allocations

```c
// For temporary allocations, use reset instead of destroy
Arena* temp_arena = arena_create(16 * 1024);

for (int i = 0; i < 1000; i++) {
    // Allocate temporary data
    void* temp_data = arena_alloc(temp_arena, 1024);
    // ... use temp_data ...
    
    // Reset for next iteration (much faster than individual frees)
    arena_reset(temp_arena);
}

arena_destroy(temp_arena);
```

## Performance Characteristics

- **Allocation speed**: Very fast for small allocations within a block
- **Memory overhead**: Minimal overhead per block (~24 bytes)
- **Fragmentation**: No internal fragmentation within blocks
- **Cache locality**: Good cache locality for sequential allocations
- **Deallocation**: O(n) where n is the number of blocks (but very fast in practice)

## Configuration

### Block Sizes

- **Default**: 64KB if no size specified
- **Minimum**: 1KB
- **Large allocations**: Automatically creates appropriately sized blocks

### Alignment

- **Default**: Pointer alignment (8 bytes on 64-bit systems)
- **Custom**: Use `arena_alloc_aligned()` for specific alignment requirements

## Best Practices

1. **Choose appropriate block sizes**: Larger blocks reduce allocation overhead but increase memory usage
2. **Use reset for temporary data**: Much faster than individual deallocations
3. **Group related allocations**: Allocate related data in the same arena for better locality
4. **Monitor memory usage**: Use the utility functions to track memory consumption
5. **Clean up properly**: Always call `arena_destroy()` when done with an arena

## Testing

Run the test suite:

```bash
make test-arena
./test_arena
```

Run the integration example:

```bash
make example-arena
./arena_example
```

## Integration with Emerald Compiler

The arena allocator is designed to integrate seamlessly with the existing Emerald compiler infrastructure:

- **AST nodes**: Allocate AST nodes in a dedicated arena
- **Symbol tables**: Use arena for symbol table entries
- **IR nodes**: Allocate IR nodes in a separate arena
- **String interning**: Efficient string storage with arena allocation
- **Temporary data**: Use arena reset for temporary allocations during compilation phases

This approach provides significant performance improvements over traditional malloc/free patterns while maintaining simplicity and reliability.
