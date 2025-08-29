#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arena.h"

// Test structure
typedef struct TestStruct {
    int id;
    char name[32];
    double value;
} TestStruct;

int main() {
    printf("=== Arena Allocator Test ===\n\n");
    
    // Create an arena with 4KB blocks
    Arena* arena = arena_create(4096);
    if (!arena) {
        fprintf(stderr, "Failed to create arena\n");
        return 1;
    }
    
    printf("Created arena with 4KB blocks\n");
    
    // Test basic allocations
    printf("\n1. Testing basic allocations:\n");
    
    int* numbers = arena_alloc_array(arena, int, 100);
    for (int i = 0; i < 100; i++) {
        numbers[i] = i * i;
    }
    printf("   Allocated 100 integers: %d, %d, %d, ..., %d\n", 
           numbers[0], numbers[1], numbers[2], numbers[99]);
    
    char* str = arena_alloc(arena, 50);
    strcpy(str, "Hello, Arena Allocator!");
    printf("   Allocated string: \"%s\"\n", str);
    
    TestStruct* test = arena_alloc_type(arena, TestStruct);
    test->id = 42;
    strcpy(test->name, "Test Object");
    test->value = 3.14159;
    printf("   Allocated struct: id=%d, name=\"%s\", value=%.5f\n", 
           test->id, test->name, test->value);
    
    // Test memory usage
    printf("\n2. Memory usage:\n");
    printf("   Used memory: %zu bytes\n", arena_get_used_memory(arena));
    printf("   Total allocated: %zu bytes\n", arena_get_total_memory(arena));
    
    // Test aligned allocations
    printf("\n3. Testing aligned allocations:\n");
    
    void* aligned_ptr = arena_alloc_aligned(arena, 64, 64);
    printf("   Aligned allocation (64-byte alignment): %p\n", aligned_ptr);
    printf("   Alignment check: %s\n", 
           ((uintptr_t)aligned_ptr % 64 == 0) ? "PASS" : "FAIL");
    
    // Test large allocation that exceeds block size
    printf("\n4. Testing large allocation:\n");
    size_t large_size = 8192;  // 8KB, larger than our 4KB blocks
    void* large_ptr = arena_alloc(arena, large_size);
    printf("   Large allocation (%zu bytes): %p\n", large_size, large_ptr);
    
    // Test memory usage after large allocation
    printf("\n5. Memory usage after large allocation:\n");
    printf("   Used memory: %zu bytes\n", arena_get_used_memory(arena));
    printf("   Total allocated: %zu bytes\n", arena_get_total_memory(arena));
    
    // Test arena reset
    printf("\n6. Testing arena reset:\n");
    size_t used_before = arena_get_used_memory(arena);
    arena_reset(arena);
    size_t used_after = arena_get_used_memory(arena);
    printf("   Memory before reset: %zu bytes\n", used_before);
    printf("   Memory after reset: %zu bytes\n", used_after);
    printf("   Reset successful: %s\n", (used_after == 0) ? "PASS" : "FAIL");
    
    // Test allocations after reset
    printf("\n7. Testing allocations after reset:\n");
    int* new_numbers = arena_alloc_array(arena, int, 50);
    for (int i = 0; i < 50; i++) {
        new_numbers[i] = i * 10;
    }
    printf("   Allocated 50 new integers: %d, %d, %d, ..., %d\n", 
           new_numbers[0], new_numbers[1], new_numbers[2], new_numbers[49]);
    
    // Final memory usage
    printf("\n8. Final memory usage:\n");
    printf("   Used memory: %zu bytes\n", arena_get_used_memory(arena));
    printf("   Total allocated: %zu bytes\n", arena_get_total_memory(arena));
    
    // Clean up
    arena_destroy(arena);
    printf("\nArena destroyed successfully\n");
    
    printf("\n=== All tests completed ===\n");
    return 0;
}
