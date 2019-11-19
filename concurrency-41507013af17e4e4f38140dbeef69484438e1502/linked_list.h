#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdlib.h>
#include <stddef.h>
#include <semaphore.h>
#include <pthread.h>

typedef struct list_node {

    struct list_node* next;
    struct list_node* prev;

    size_t data;
    int op;
    size_t index;
    int* opera;
    void** select;

    sem_t* list_sem;

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
list_node_t* list_find(list_t* list, int op);

// Insert a new node in the list with the given data
void list_insert(list_t* list, size_t data, int op, sem_t* list_sem, size_t index, int* opera, void** select);

// Remove a node from the list and free the node resources
void list_remove(list_t* list, list_node_t* node);

// Execute a function for each element in the list
void list_foreach(list_t* list, void (*func)(void* data));

list_node_t* node_delete(list_t* list, sem_t* list_sem);

list_node_t* node_index(list_t* list, sem_t* list_sem, int op);

#endif
