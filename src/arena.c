#include "arena.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Default block size if none specified
#define DEFAULT_BLOCK_SIZE (64 * 1024)  // 64KB
#define MIN_BLOCK_SIZE 1024              // 1KB minimum

// Calculate alignment padding
static size_t align_up(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

// Create a new arena allocator
Arena* arena_create(size_t block_size) {
    if (block_size == 0) {
        block_size = DEFAULT_BLOCK_SIZE;
    } else if (block_size < MIN_BLOCK_SIZE) {
        block_size = MIN_BLOCK_SIZE;
    }
    
    Arena* arena = malloc(sizeof(Arena));
    if (!arena) {
        return NULL;
    }
    
    arena->block_size = block_size;
    arena->alignment = sizeof(void*);  // Default to pointer alignment
    arena->current_block = NULL;
    
    return arena;
}

// Allocate memory from the arena
void* arena_alloc(Arena* arena, size_t size) {
    if (!arena || size == 0) {
        return NULL;
    }
    
    // Align the requested size
    size_t aligned_size = align_up(size, arena->alignment);
    
    // Check if current block has enough space
    if (arena->current_block && 
        arena->current_block->used + aligned_size <= arena->current_block->capacity) {
        void* ptr = arena->current_block->data + arena->current_block->used;
        arena->current_block->used += aligned_size;
        return ptr;
    }
    
    // Need to allocate a new block
    size_t block_size = arena->block_size;
    if (aligned_size > block_size) {
        // For large allocations, create a block just for this allocation
        block_size = aligned_size;
    }
    
    ArenaBlock* new_block = malloc(sizeof(ArenaBlock) + block_size);
    if (!new_block) {
        return NULL;
    }
    
    new_block->next = arena->current_block;
    new_block->used = aligned_size;
    new_block->capacity = block_size;
    
    arena->current_block = new_block;
    
    return new_block->data;
}

// Allocate aligned memory from the arena
void* arena_alloc_aligned(Arena* arena, size_t size, size_t alignment) {
    if (!arena || size == 0 || alignment == 0) {
        return NULL;
    }
    
    // Ensure alignment is a power of 2
    if ((alignment & (alignment - 1)) != 0) {
        return NULL;
    }
    
    // Calculate aligned size
    size_t aligned_size = align_up(size, alignment);
    
    // Check if current block has enough space with alignment
    if (arena->current_block) {
        size_t current_offset = arena->current_block->used;
        size_t aligned_offset = align_up(current_offset, alignment);
        // size_t total_needed = aligned_offset - current_offset + aligned_size;
        
        if (aligned_offset + aligned_size <= arena->current_block->capacity) {
            // Update used to account for alignment padding
            arena->current_block->used = aligned_offset + aligned_size;
            return arena->current_block->data + aligned_offset;
        }
    }
    
    // Need to allocate a new block
    size_t block_size = arena->block_size;
    if (aligned_size > block_size) {
        block_size = aligned_size;
    }
    
    ArenaBlock* new_block = malloc(sizeof(ArenaBlock) + block_size);
    if (!new_block) {
        return NULL;
    }
    
    new_block->next = arena->current_block;
    new_block->used = aligned_size;
    new_block->capacity = block_size;
    
    arena->current_block = new_block;
    
    return new_block->data;
}

// Reset the arena (free all blocks but keep the arena structure)
void arena_reset(Arena* arena) {
    if (!arena) {
        return;
    }
    
    ArenaBlock* current = arena->current_block;
    while (current) {
        ArenaBlock* next = current->next;
        free(current);
        current = next;
    }
    
    arena->current_block = NULL;
}

// Destroy the arena and free all memory
void arena_destroy(Arena* arena) {
    if (!arena) {
        return;
    }
    
    arena_reset(arena);
    free(arena);
}

// Get the total used memory across all blocks
size_t arena_get_used_memory(Arena* arena) {
    if (!arena) {
        return 0;
    }
    
    size_t total = 0;
    ArenaBlock* current = arena->current_block;
    
    while (current) {
        total += current->used;
        current = current->next;
    }
    
    return total;
}

// Get the total allocated memory across all blocks
size_t arena_get_total_memory(Arena* arena) {
    if (!arena) {
        return 0;
    }
    
    size_t total = 0;
    ArenaBlock* current = arena->current_block;
    
    while (current) {
        total += current->capacity;
        current = current->next;
    }
    
    return total;
}
