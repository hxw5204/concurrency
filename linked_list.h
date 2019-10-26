#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdlib.h>
#include <stddef.h>

typedef struct list_node {
    struct list_node* next;
    struct list_node* prev;
    void* data;
} list_node_t;

typedef struct {
    list_node_t* head;
    size_t count;
} list_t;

// Create and return a new list
list_t* list_create();

// Destroy a list
void list_destroy(list_t* list);

// Return the number of elements in the list
size_t list_count(list_t* list);

// Find the first node in the list with the given data
// Returns NULL if data could not be found
list_node_t* list_find(list_t* list, void* data);

// Insert a new node in the list with the given data
void list_insert(list_t* list, void* data);

// Remove a node from the list and free the node resources
void list_remove(list_t* list, list_node_t* node);

// Execute a function for each element in the list
void list_foreach(list_t* list, void (*func)(void* data));

#endif
