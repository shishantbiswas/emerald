#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>

// Arena block structure
typedef struct ArenaBlock {
    struct ArenaBlock* next;
    size_t used;
    size_t capacity;
    uint8_t data[];
} ArenaBlock;

// Arena allocator structure
typedef struct Arena {
    ArenaBlock* current_block;
    size_t block_size;
    size_t alignment;
} Arena;

// Function declarations
Arena* arena_create(size_t block_size);
void* arena_alloc(Arena* arena, size_t size);
void* arena_alloc_aligned(Arena* arena, size_t size, size_t alignment);
void arena_reset(Arena* arena);
void arena_destroy(Arena* arena);

// Convenience macros for common allocations
#define arena_alloc_type(arena, type) ((type*)arena_alloc(arena, sizeof(type)))
#define arena_alloc_array(arena, type, count) ((type*)arena_alloc(arena, sizeof(type) * (count)))

// Utility functions
size_t arena_get_used_memory(Arena* arena);
size_t arena_get_total_memory(Arena* arena);

#endif // ARENA_H
