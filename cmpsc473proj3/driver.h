#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "queue.h"
#include "linked_list.h"
#include <stddef.h>
#include <string.h>

typedef struct driver
{
    // DO NOT REMOVE queue (OR CHANGE ITS NAME) FROM THE STRUCT
    // YOU MUST USE queue TO STORE YOUR QUEUEERED JOBS
    queue_t *queue;
    /* ADD ANY STRUCT ENTRIES YOU NEED HERE */
    pthread_mutex_t lock;
    int driver_closed;

    pthread_cond_t empty;
    pthread_cond_t full;
    pthread_mutex_t mutex;

    void *unqueuedJob;
    size_t capacity;
    list_t *list;
    int count;
    /* IMPLEMENT THIS */

} driver_t;

enum driver_status
{
    DRIVER_REQUEST_EMPTY = 0,
    DRIVER_REQUEST_FULL = 0,
    SUCCESS = 1,
    DRIVER_CLOSED_ERROR = -2,
    DRIVER_GEN_ERROR = -1,
    DRIVER_DESTROY_ERROR = -3
};

enum operation
{
    SCHDLE,
    HANDLE,
};

typedef struct
{
    driver_t *driver;
    enum operation op;
    void *job;
} select_t;

driver_t *driver_create(size_t size);

enum driver_status driver_schedule(driver_t *driver, void *job);

enum driver_status driver_handle(driver_t *driver, void **job);

enum driver_status driver_non_blocking_schedule(driver_t *driver, void *job);

enum driver_status driver_non_blocking_handle(driver_t *driver, void **job);

enum driver_status driver_close(driver_t *driver);

enum driver_status driver_destroy(driver_t *driver);

enum driver_status driver_select(select_t *driver_list, size_t driver_count, size_t *selected_index);

void wake_select(void *job);

list_node_t *helper(list_t *list, pthread_cond_t *con_addr);

#endif
