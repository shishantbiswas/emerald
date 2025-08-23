#include <stdlib.h>
#include "../include/linked_list.h"

// Function to create a new linked list node
Node* createNode(int data) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        return NULL;  // Memory allocation failed
    }
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}

// Function to insert a node at the beginning of the linked list
void insertNode(Node** head, int data) {
    Node* newNode = createNode(data);
    if (newNode == NULL) {
        return;  // Memory allocation failed
    }
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
    if (*head == NULL) {
        return;  // List is empty
    }

    Node* current = *head;
    Node* prev = NULL;

    // If the node to be deleted is the head
    if (current != NULL && current->data == data) {
        *head = current->next;
        free(current);
        return;
    }

    // Search for the node to be deleted
    while (current != NULL && current->data != data) {
        prev = current;
        current = current->next;
    }

    // If the node wasn't found
    if (current == NULL) {
        return;
    }

    // Unlink the node from the list
    prev->next = current->next;
    free(current);
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
