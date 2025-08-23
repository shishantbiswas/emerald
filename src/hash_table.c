#include <stdlib.h>
#include <stdio.h>
#include "../include/hash_table.h"

// Internal hash function
static int hash(int key, int size) {
    return key % size;
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
void insertEntry(Hashtable* hashtable, int key, int value) {
    if (hashtable == NULL || hashtable->table == NULL) {
        return;  // Invalid hashtable
    }

    int index = hash(key, hashtable->size);
    
    // Create new entry
    Entry* newEntry = (Entry*)malloc(sizeof(Entry));
    if (newEntry == NULL) {
        return;  // Memory allocation failed
    }
    
    newEntry->key = key;
    newEntry->value = value;
    newEntry->next = hashtable->table[index];
    hashtable->table[index] = newEntry;
}

// Function to search for a value by key in the hashtable
int searchEntry(Hashtable* hashtable, int key) {
    if (hashtable == NULL || hashtable->table == NULL) {
        return -1;  // Invalid hashtable or not found
    }

    int index = hash(key, hashtable->size);
    Entry* current = hashtable->table[index];
    
    while (current != NULL) {
        if (current->key == key) {
            return current->value;
        }
        current = current->next;
    }
    
    return -1; // Not found
}

// Function to delete an entry from the hashtable
void deleteEntry(Hashtable* hashtable, int key) {
    if (hashtable == NULL || hashtable->table == NULL) {
        return;  // Invalid hashtable
    }

    int index = hash(key, hashtable->size);
    Entry* current = hashtable->table[index];
    Entry* prev = NULL;
    
    while (current != NULL && current->key != key) {
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
    
    free(current);
}

// Function to free the entire hashtable
void freeHashtable(Hashtable* hashtable) {
    if (hashtable == NULL) {
        return;  // Nothing to free
    }
    
    if (hashtable->table != NULL) {
        // Free all entries in the table
        for (int i = 0; i < hashtable->size; i++) {
            Entry* current = hashtable->table[i];
            while (current != NULL) {
                Entry* temp = current;
                current = current->next;
                free(temp);
            }
        }
        free(hashtable->table);
    }
    
    free(hashtable);
}
