#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Linked List Node
typedef struct Node {
    int data;
    struct Node* next;
} Node;

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

// Function to create a new linked list node
Node* createNode(int data) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}

// Function to insert a node at the beginning of the linked list
void insertNode(Node** head, int data) {
    Node* newNode = createNode(data);
    newNode->next = *head;
    *head = newNode;
}

// Function to search for a node in the linked list
Node* searchNode(Node* head, int data) {
    Node* current = head;
    while (current != NULL) {
        if (current->data == data) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Function to delete a node from the linked list
void deleteNode(Node** head, int data) {
    Node* current = *head;
    Node* prev = NULL;
    while (current != NULL && current->data != data) {
        prev = current;
        current = current->next;
    }
    if (current == NULL) return;
    if (prev == NULL) {
        *head = current->next;
    } else {
        prev->next = current->next;
    }
    free(current);
}

// Function to create a new hashtable
Hashtable* createHashtable(int size) {
    Hashtable* hashtable = (Hashtable*)malloc(sizeof(Hashtable));
    hashtable->size = size;
    hashtable->table = (Entry**)malloc(sizeof(Entry*) * size);
    for (int i = 0; i < size; i++) {
        hashtable->table[i] = NULL;
    }
    return hashtable;
}

// Hash function
int hash(int key, int size) {
    return key % size;
}

// Function to insert a key-value pair into the hashtable
void insertEntry(Hashtable* hashtable, int key, int value) {
    int index = hash(key, hashtable->size);
    Entry* newEntry = (Entry*)malloc(sizeof(Entry));
    newEntry->key = key;
    newEntry->value = value;
    newEntry->next = hashtable->table[index];
    hashtable->table[index] = newEntry;
}

// Function to search for a value by key in the hashtable
int searchEntry(Hashtable* hashtable, int key) {
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
    int index = hash(key, hashtable->size);
    Entry* current = hashtable->table[index];
    Entry* prev = NULL;
    while (current != NULL && current->key != key) {
        prev = current;
        current = current->next;
    }
    if (current == NULL) return;
    if (prev == NULL) {
        hashtable->table[index] = current->next;
    } else {
        prev->next = current->next;
    }
    free(current);
}

// Function to free the entire hashtable
void freeHashtable(Hashtable* hashtable) {
    for (int i = 0; i < hashtable->size; i++) {
        Entry* current = hashtable->table[i];
        while (current != NULL) {
            Entry* temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(hashtable->table);
    free(hashtable);
}

// Function to free the entire linked list
void freeLinkedList(Node* head) {
    Node* current = head;
    while (current != NULL) {
        Node* temp = current;
        current = current->next;
        free(temp);
    }
}
