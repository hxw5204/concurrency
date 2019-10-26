#ifndef BUFFER_H
#define BUFFER_H

#include <stdlib.h>

typedef struct {
    size_t size;
    size_t next;
    size_t capacity;
    void** job;
} queue_t;

enum queue_status {
    QUEUE_SUCCESS = 1,
    QUEUE_ERROR = -1
};

// Creates a queue with the given capacity
queue_t* queue_create(size_t capacity);

// Adds the value into the queue
// This function returns BUFF_SUCCESS if the queue is full
// Otherwise, it returns BUFF_ERROR
enum queue_status queue_add(queue_t* queue, void* job);

// Removes the value from the queue in FIFO order and stores it in job
// This function returns BUFF_SUCCESS if the queue is non-empty
// Otherwise, it returns BUFF_ERROR
enum queue_status queue_remove(queue_t* queue, void** job);

// Frees the memory allocated to the queue
void queue_free(queue_t* queue);

// Returns the total capacity of the queue
size_t queue_capacity(queue_t* queue);

// Returns the current number of elements in the queue
size_t queue_current_size(queue_t* queue);

// Peeks at a value in the queue
// Only used for testing code; you should not need this
void* peek_queue(queue_t* queue, size_t index);

#endif
