#ifndef LINKED_LIST_H
#define LINKED_LIST_H

// Linked List Node
typedef struct Node {
    int data;
    struct Node* next;
} Node;

// Function declarations
Node* createNode(int data);
void insertNode(Node** head, int data);
Node* searchNode(Node* head, int data);
void deleteNode(Node** head, int data);
void freeLinkedList(Node* head);

#endif // LINKED_LIST_H
