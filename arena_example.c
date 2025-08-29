#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arena.h"
#include "hash_table.h"

// Arena-aware hash table functions
Hashtable* createHashtableArena(Arena* arena, int size) {
    if (size <= 0) {
        return NULL;
    }

    Hashtable* hashtable = arena_alloc_type(arena, Hashtable);
    if (hashtable == NULL) {
        return NULL;
    }

    hashtable->size = size;
    hashtable->table = arena_alloc_array(arena, Entry*, size);
    
    if (hashtable->table == NULL) {
        return NULL;
    }
    
    // Initialize all entries to NULL
    for (int i = 0; i < size; i++) {
        hashtable->table[i] = NULL;
    }
    
    return hashtable;
}

void insertEntryArena(Hashtable* hashtable, char* data, Arena* arena) {
    if (hashtable == NULL || hashtable->table == NULL || data == NULL) {
        return;
    }

    // Calculate hash and ensure it's within table bounds
    unsigned int h = hash(data);
    int index = h % hashtable->size;
    
    // Create new entry using arena
    Entry* newEntry = arena_alloc_type(arena, Entry);
    if (newEntry == NULL) {
        return;
    }
    
    // Allocate space for the string in the arena
    size_t data_len = strlen(data) + 1;
    newEntry->data = arena_alloc(arena, data_len);
    if (newEntry->data == NULL) {
        return;
    }
    
    strcpy(newEntry->data, data);
    newEntry->next = hashtable->table[index];
    hashtable->table[index] = newEntry;
}

int main() {
    printf("=== Arena + Hash Table Integration Example ===\n\n");
    
    // Create an arena for all allocations
    Arena* arena = arena_create(8192);  // 8KB blocks
    if (!arena) {
        fprintf(stderr, "Failed to create arena\n");
        return 1;
    }
    
    printf("Created arena with 8KB blocks\n");
    
    // Create a hash table using the arena
    Hashtable* ht = createHashtableArena(arena, 16);
    if (!ht) {
        fprintf(stderr, "Failed to create hash table\n");
        arena_destroy(arena);
        return 1;
    }
    
    printf("Created hash table with 16 buckets\n");
    
    // Insert some test data
    const char* test_data[] = {
        "hello", "world", "emerald", "compiler", "arena", "allocator",
        "hash", "table", "memory", "management", "fast", "efficient"
    };
    
    printf("\nInserting test data:\n");
    for (int i = 0; i < 12; i++) {
        insertEntryArena(ht, (char*)test_data[i], arena);
        printf("  Inserted: \"%s\"\n", test_data[i]);
    }
    
    // Search for some data
    printf("\nSearching for data:\n");
    const char* search_terms[] = {"hello", "compiler", "missing", "arena"};
    
    for (int i = 0; i < 4; i++) {
        char* result = searchEntry(ht, (char*)search_terms[i]);
        if (result) {
            printf("  Found: \"%s\"\n", result);
        } else {
            printf("  Not found: \"%s\"\n", search_terms[i]);
        }
    }
    
    // Show memory usage
    printf("\nMemory usage:\n");
    printf("  Used memory: %zu bytes\n", arena_get_used_memory(arena));
    printf("  Total allocated: %zu bytes\n", arena_get_total_memory(arena));
    
    // Demonstrate arena reset
    printf("\nResetting arena (this would free all hash table data):\n");
    arena_reset(arena);
    printf("  Used memory after reset: %zu bytes\n", arena_get_used_memory(arena));
    
    // Create a new hash table after reset
    printf("\nCreating new hash table after reset:\n");
    Hashtable* ht2 = createHashtableArena(arena, 8);
    insertEntryArena(ht2, "fresh", arena);
    insertEntryArena(ht2, "data", arena);
    
    char* result = searchEntry(ht2, "fresh");
    printf("  Found after reset: \"%s\"\n", result ? result : "NULL");
    
    // Clean up
    arena_destroy(arena);
    printf("\nArena destroyed (all memory freed)\n");
    
    printf("\n=== Example completed ===\n");
    return 0;
}
