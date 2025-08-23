#ifndef HASH_TABLE_H
#define HASH_TABLE_H

// Hashtable Entry
typedef struct Entry {
    int key;
    int value;
    struct Entry* next;
} Entry;

// Hashtable
typedef struct Hashtable {
    int size;
    Entry** table;
} Hashtable;

// Function declarations
Hashtable* createHashtable(int size);
void insertEntry(Hashtable* hashtable, int key, int value);
int searchEntry(Hashtable* hashtable, int key);
void deleteEntry(Hashtable* hashtable, int key);
void freeHashtable(Hashtable* hashtable);

// Internal helper function (only used within hash_table.c)
static int hash(int key, int size);

#endif // HASH_TABLE_H
