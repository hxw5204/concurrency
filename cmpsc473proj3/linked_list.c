#include "linked_list.h"
#include "driver.h"
#include <pthread.h>
// Create and return a new list
list_t *list_create()
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    list_t *list;
    list = (list_t *)malloc(sizeof(list_t));
    list->head = NULL;
    list->count = 0;

    return list;
}

// Destroy a list
void list_destroy(list_t *list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    list_node_t *temp;

    if (list == NULL)
    {
        return;
    }
    else
    {
        while (list->head != NULL)
        {
            temp = list->head;
            list->head = list->head->next;
            free(temp);
        }

        free(list);
        list = NULL;
        return;
    }
}

// Return the number of elements in the list
size_t list_count(list_t *list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    return list->count;
}

// Find the first node in the list with the given data
// Returns NULL if data could not be found
list_node_t *list_find(list_t *list, int op)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    if (list->count == 0)
    {
        return NULL;
    }
    else
    {
        list_node_t *node = list->head;
        while (node->op != op)
        {
            node = node->next;
            if (node == NULL)
            {
                return NULL;
            }
        }
        return node;
    }
}

// Insert a new node in the list with the given data
void list_insert(list_t *list, size_t data, int op, pthread_cond_t *cv, size_t *index, int *opera, void **select)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    list_node_t *node;
    node = (list_node_t *)malloc(sizeof(list_node_t));
    if (list->count == 0)
    {
        list->head = node;
        list->head->next = NULL;
        list->head->prev = NULL;
        list->head->data = data;

        list->head->op = op;

        list->head->cv = cv;
        list->head->index = index;
        list->head->opera = opera;

        list->head->select = select;
    }
    else
    {
        node->next = NULL;
        list_node_t *temp;
        temp = list->head;
        while (temp->next != NULL)
        {
            temp = temp->next;
        }
        node->prev = temp;
        temp->next = node;

        node->data = data;
        node->op = op;
        node->cv = cv;

        node->index = index;
        node->opera = opera;
        node->select = select;
    }
    list->count++;
}

// Remove a node from the list and free the node resources
void list_remove(list_t *list, list_node_t *node)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    if (list == NULL)
    {
        return;
    }
    else if (list->count == 1)
    {
        if (node != NULL)
        {
            free(node);
        }

        list->count = 0;
        list->head = NULL;
    }
    else if (list->head == node)
    {
        node->next->prev = NULL;
        list->head = node->next;
        free(node);
        list->count--;
    }
    else
    {
        node->prev->next = node->next;
        if (node->next != NULL)
        {
            node->next->prev = node->prev;
        }
        free(node);
        list->count--;
    }
}

// Execute a function for each element in the list
void list_foreach(list_t *list, void (*func)(void *data))
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
}
