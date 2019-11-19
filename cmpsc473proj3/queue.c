#include "queue.h"

// Creates a queue with the given capacity
queue_t* queue_create(size_t capacity)
{
    queue_t* queue = (queue_t*) malloc(sizeof(queue_t));
    void** job  = (void**) malloc(capacity * sizeof(void*));
    queue->size = 0;
    queue->next = 0;
    queue->capacity = capacity;
    queue->job = job;
    return queue;
}

// Adds the value into the queue
// This function returns QUEUE_SUCCESS if the queue is full
// Otherwise, it returns QUEUE_ERROR
enum queue_status queue_add(queue_t* queue, void* job)
{
    if (queue->size >= queue->capacity) {
        return QUEUE_ERROR;
    }
    size_t pos = queue->next + queue->size;
    if (pos >= queue->capacity) {
        pos -= queue->capacity;
    }
    queue->job[pos] = job;
    queue->size++;
    return QUEUE_SUCCESS;
}

// Removes the value from the queue in FIFO order and stores it in job
// This function returns QUEUE_SUCCESS if the queue is non-empty
// Otherwise, it returns QUEUE_ERROR
enum queue_status queue_remove(queue_t* queue, void **job)
{
    if (queue->size > 0) {
        *job = queue->job[queue->next];
        queue->size--;
        queue->next++;
        if (queue->next >= queue->capacity) {
            queue->next -= queue->capacity;
        }
        return QUEUE_SUCCESS;
    }
    return QUEUE_ERROR;
}

// Frees the memory allocated to the queue
void queue_free(queue_t *queue)
{
    free(queue->job);
    free(queue);
}

// Returns the total capacity of the queue
size_t queue_capacity(queue_t* queue)
{
    return queue->capacity;
}

// Returns the current number of elements in the queue
size_t queue_current_size(queue_t* queue)
{
    return queue->size;
}

// Peeks at a value in the queue
// Only used for testing code; you should not need this
void* peek_queue(queue_t* queue, size_t index)
{
    return queue->job[index];
}
