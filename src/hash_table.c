#include <stdlib.h>
#include <stdio.h>
#include "hash_table.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// djb2 hash function by Dan Bernstein
int hash(const char *str) {
    int h = 5381;
    int c;

    while ((c = *str++)) {
        h = ((h << 5) + h) + c;
    }

    return h;
}

// Function to create a new hashtable
Hashtable* createHashtable(int size) {
    if (size <= 0) {
        return NULL;  // Invalid size
    }

    Hashtable* hashtable = (Hashtable*)malloc(sizeof(Hashtable));
    if (hashtable == NULL) {
        return NULL;  // Memory allocation failed
    }

    hashtable->size = size;
    hashtable->table = (Entry**)calloc(size, sizeof(Entry*));
    
    if (hashtable->table == NULL) {
        free(hashtable);
        return NULL;  // Memory allocation failed
    }
    
    return hashtable;
}

// Function to insert a key-value pair into the hashtable
void insertEntry(Hashtable* hashtable, char* data) {
    if (hashtable == NULL || hashtable->table == NULL || data == NULL) {
        return;  // Invalid parameters
    }

    // Calculate hash and ensure it's within table bounds
    unsigned int h = hash(data);
    int index = h % hashtable->size;
    
    // Create new entry
    Entry* newEntry = (Entry*)malloc(sizeof(Entry));
    if (newEntry == NULL) {
        return;  // Memory allocation failed
    }
    
    newEntry->data = strdup(data);  // Make a copy of the string
    if (newEntry->data == NULL) {
        free(newEntry);
        return;  // Failed to duplicate string
    }
    
    newEntry->next = hashtable->table[index];
    hashtable->table[index] = newEntry;
}

// Function to search for a value by key in the hashtable
char* searchEntry(Hashtable* hashtable, char* data) {
    if (hashtable == NULL || hashtable->table == NULL || data == NULL) {
        return NULL;  // Invalid parameters
    }

    unsigned int h = hash(data);
    int index = h % hashtable->size;
    Entry* current = hashtable->table[index];
    
    while (current != NULL) {
        if (strcmp(current->data, data) == 0) {
            return current->data;
        }
        current = current->next;
    }
    
    return NULL; // Not found
}

// Function to delete an entry from the hashtable
void deleteEntry(Hashtable* hashtable, char* data) {
    if (hashtable == NULL || hashtable->table == NULL || data == NULL) {
        return;  // Invalid parameters
    }

    unsigned int h = hash(data);
    int index = h % hashtable->size;
    Entry* current = hashtable->table[index];
    Entry* prev = NULL;
    
    while (current != NULL) {
        if (strcmp(current->data, data) == 0) {
            break;
        }
        prev = current;
        current = current->next;
    }
    
    if (current == NULL) {
        return;  // Key not found
    }
    
    // Remove the node
    if (prev == NULL) {
        // Remove first entry
        hashtable->table[index] = current->next;
    } else {
        prev->next = current->next;
    }
    
    // Free the duplicated string and the entry
    if (current->data != NULL) {
        free(current->data);
    }
    free(current);
}

// Function to free the entire hashtable
void freeHashtable(Hashtable* hashtable) {
    if (hashtable == NULL) {
        return;  // Nothing to free
    }
    
    if (hashtable->table != NULL) {
        // Free all entries
        for (int i = 0; i < hashtable->size; i++) {
            Entry* current = hashtable->table[i];
            while (current != NULL) {
                Entry* temp = current;
                current = current->next;
                if (temp->data != NULL) {
                    free(temp->data);  // Free the duplicated string
                }
                free(temp);
            }
        }
        
        // Free the table
        free(hashtable->table);
        hashtable->table = NULL;
    }
    
    // Free the hashtable structure
    free(hashtable);
}
