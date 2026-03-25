#ifndef HASH_TABLE_H
#define HASH_TABLE_H

// Hashtable Entry
typedef struct Entry {
    char* key;
    char* data;
    int is_string;
    struct Entry* next;
} Entry;

// Hashtable
typedef struct Hashtable {
    int size;
    Entry** table;
} Hashtable;

// Function declarations
Hashtable* createHashtable(int size);
void insertEntry(Hashtable* hashtable, char* key, void* data, int is_string);
void* findEntry(Hashtable* hashtable, char* key);
char* searchEntry(Hashtable* hashtable, char* data); // For string lookup
void deleteEntry(Hashtable* hashtable, char* data);
void freeHashtable(Hashtable* hashtable);

// Internal helper function (only used within hash_table.c)
int hash(const char *str);

#endif // HASH_TABLE_H
