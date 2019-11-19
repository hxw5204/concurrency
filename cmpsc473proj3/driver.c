#include "driver.h"
#include <pthread.h>
#include "linked_list.h"

//sem_t sem_driver;
sem_t full;
sem_t empty;

/*
	Creates a new driver with the provided job queue size and returns it to the caller function.
	A 0 size indicates an unqueued driver, whereas a positive size indicates a queued driver.
*/

driver_t *driver_create(size_t size)
{
    /* IMPLEMENT THIS */
    driver_t *driver = (driver_t *)malloc(sizeof(driver_t));
    driver->queue = queue_create(size);
    driver->driver_closed = 0;

    //sem_init(&empty, 0, 1);
    //sem_init(&full, 0, 0);

    pthread_mutex_init(&(driver->mutex), NULL);
    pthread_cond_init(&(driver->empty), NULL);
    pthread_cond_init(&(driver->full), NULL);

    driver->count = 0;
    driver->list = NULL;
    driver->capacity = size;
    driver->unqueuedJob = NULL;

    return driver;
}

// Checks if the given driver has space to accommodate the new job and schedule it. This is a blocking call,
// the function only returns on a successful completion of schedule. In case the queue is full, the function
// waits till the queue is available to take in new job. The return type is enum driver_status as defined in griver.h.
// Return
// 		- SUCCESS for successful queuing of job,
// 		- DRIVER_CLOSED_ERROR when the queue is closed
// 		- DRIVER_GEN_ERROR on encountering any other generic error of any sort.
enum driver_status driver_schedule(driver_t *driver, void *job)
{
    /* IMPLEMENT THIS */
    /*
    if (!driver)
    {
        return DRIVER_GEN_ERROR;
    }
    if (driver->status == DRIVER_CLOSED_ERROR)
    {
        return DRIVER_CLOSED_ERROR;
    }
    while (1)
    {
        // -1 from sem, if the sem is not 0
        sem_wait(&empty);
        pthread_mutex_lock(&mutex); // blocking lock

        //  if successful have mutex, then add job
        if (queue_add(driver->queue, job) == QUEUE_SUCCESS)
        {
            pthread_mutex_unlock(&mutex); // unlock
            sem_post(&full);
            driver->status = SUCCESS;
            return SUCCESS;
        }
        else
        {
            pthread_mutex_unlock(&mutex);
            driver->status = DRIVER_GEN_ERROR;
            return DRIVER_GEN_ERROR;
        }
    }
    */

    pthread_mutex_lock(&(driver->mutex));

    if (driver->driver_closed)
    {

        pthread_mutex_unlock(&(driver->mutex));
        return DRIVER_CLOSED_ERROR;
    }
    else if (driver->capacity != 0)
    {

        while (driver->queue->size == driver->capacity)
        {

            pthread_cond_wait(&(driver->full), &(driver->mutex));

            if (driver->driver_closed)
            {

                pthread_mutex_unlock(&(driver->mutex));
                return DRIVER_CLOSED_ERROR;
            }
        }

        if (queue_add(driver->queue, job) == QUEUE_SUCCESS)
        {

            if (driver->list != NULL)
            {

                list_node_t *node;

                node = list_find(driver->list, 2);
                if (node != NULL)
                {

                    *(node->index) = node->data;
                    *(node->opera) = node->op;
                    *(node->select) = (void *)driver;
                    pthread_cond_signal(node->cv);
                }
                else
                {
                    pthread_cond_signal(&(driver->empty));
                }
            }
            else
            {
                pthread_cond_signal(&(driver->empty));
            }
        }
        else
        {
            pthread_mutex_unlock(&(driver->mutex));
            return DRIVER_GEN_ERROR;
        }
    }
    else
    {

        //unqueued
        int i = 0;

        if ((driver->count) >= 0)
        {

            i = 1;
            driver->count++;
            pthread_cond_wait(&(driver->full), &(driver->mutex));
        }
        if (driver->driver_closed)
        {
            pthread_mutex_unlock(&(driver->mutex));
            return DRIVER_CLOSED_ERROR;
        }
        if (i == 0)
        {

            driver->count++;
        }
        driver->unqueuedJob = job;
        if (driver->unqueuedJob != job)
        {

            pthread_mutex_unlock(&(driver->mutex));
            return DRIVER_GEN_ERROR;
        }

        pthread_cond_signal(&(driver->empty));
    }
    pthread_mutex_unlock(&(driver->mutex));

    return SUCCESS;
}

/*
	driver_handle: Picks up data from the given driver and stores it in the function’s input
	parameter, job (Note that it is a double pointer). This is a blocking call, i.e., the function only
	returns on a successful job completion. In case the queue is empty, the function waits until
	the queue has some jobs to pick up. The return type is enum driver_status as defined in
	driver.h. Return
		o SUCCESS for successful retrieval of job,
		o DRIVER_CLOSED_ERROR when the driver is closed, and
		o DRIVER_GEN_ERROR on encountering any other generic error of any sort. 
*/
enum driver_status driver_handle(driver_t *driver, void **job)
{
    /* IMPLEMENT THIS */

    /*
	if (driver->status == DRIVER_CLOSED_ERROR){
		return DRIVER_CLOSED_ERROR;
	}
	while(1){
		// -1 from sem, if the sem is not 0
		sem_wait(&full); 
		pthread_mutex_lock(&mutex);// blocking lock
		
		//  if successful have mutex, then add job
		if (queue_remove(driver->queue, job) == QUEUE_SUCCESS){
			driver->status = SUCCESS;
			pthread_mutex_unlock(&mutex); // unlock
			sem_post(&empty);
		
			return SUCCESS;
		}else{
			driver->status = DRIVER_GEN_ERROR;
			pthread_mutex_unlock(&mutex);
			driver->status = DRIVER_GEN_ERROR;
			return DRIVER_GEN_ERROR;
		}
	}
	*/

    pthread_mutex_lock(&(driver->mutex));

    if (driver->driver_closed)
    {
        pthread_mutex_unlock(&(driver->mutex));
        return DRIVER_CLOSED_ERROR;
    }
    if (driver->capacity != 0)
    {
        while (driver->queue->size == 0)
        {

            pthread_cond_wait(&(driver->empty), &(driver->mutex));

            if (driver->driver_closed)
            {
                pthread_mutex_unlock(&(driver->mutex));
                return DRIVER_CLOSED_ERROR;
            }
        }

        if (queue_remove(driver->queue, job) == QUEUE_SUCCESS)
        {

            if (driver->list != NULL)
            {

                list_node_t *node;
                node = list_find(driver->list, 1);
                if (node != NULL)
                {
                    *(node->index) = node->data;
                    *(node->opera) = node->op;
                    *(node->select) = (void *)driver;
                    pthread_cond_signal(node->cv);
                }
                else
                {
                    pthread_cond_signal(&(driver->full));
                }
            }
            else
            {
                pthread_cond_signal(&(driver->full));
            }
        }
        else
        {
            pthread_mutex_unlock(&(driver->mutex));
            return DRIVER_GEN_ERROR;
        }
    }
    else
    {
        //unqueued

        driver->count--;

        pthread_cond_signal(&(driver->full));

        pthread_cond_wait(&(driver->empty), &(driver->mutex));

        if (driver->driver_closed)
        {
            pthread_mutex_unlock(&(driver->mutex));
            return DRIVER_CLOSED_ERROR;
        }
        *job = driver->unqueuedJob;
        if (*job == driver->unqueuedJob)
        {

            driver->unqueuedJob = NULL;
        }
        else
        {
            pthread_mutex_unlock(&(driver->mutex));
            return DRIVER_GEN_ERROR;
        }
    }

    pthread_mutex_unlock(&(driver->mutex));

    return SUCCESS;
}

/*
	driver_non_blocking_schedule: Checks if the given driver has space to accommodate the new
	job and populates it. This is a non-blocking call, i.e., the function simply returns if the queue
	is full. The return type is enum driver_status as defined in driver.h. Return
		o SUCCESS for successful queuing of job,
		o DRIVER_FULL if the queue is full and the data was not added to the queue,
		o DRIVER_CLOSED_ERROR when the driver is closed, and
		o DRIVER_GEN_ERROR on encountering any other generic error of any sort.
*/

enum driver_status driver_non_blocking_schedule(driver_t *driver, void *job)
{
    /* IMPLEMENT THIS */
    /*
	if (!driver){
		return DRIVER_GEN_ERROR;
	}
	if (driver->status == DRIVER_CLOSED_ERROR){
		return DRIVER_CLOSED_ERROR;
	}
	while(1){
		// -1 from sem, if the sem is not 0
		ret = sem_trywait(&empty); 
		if ((ret == -1) && (errno == EAGAIN)){
			driver->status = DRIVER_REQUEST_FULL;
			return DRIVER_REQUEST_FULL;
		}else if((ret == -1) && ((errno == EINTR) || (errno == EINVAL))){
			driver->status = DRIVER_GEN_ERROR;
			return DRIVER_GEN_ERROR;
		}else{
			//pthread_mutex_trylock(&mutex);// non-blocking lock
			
			//  if successful have mutex, then add job
			if (queue_add(driver->queue, job) == QUEUE_SUCCESS){
				//pthread_mutex_unlock(&mutex); // unlock
				sem_post(&full);
				driver->status = SUCCESS;
				return SUCCESS;
			}else{
				//pthread_mutex_unlock(&mutex);
				driver->status = DRIVER_GEN_ERROR;
				return DRIVER_GEN_ERROR;
			}
		}
	}
	*/

    pthread_mutex_lock(&(driver->mutex));
    if (driver->driver_closed)
    {
        pthread_mutex_unlock(&(driver->mutex));
        return DRIVER_CLOSED_ERROR;
    }
    if (driver->capacity != 0)
    {

        if (driver->queue->size == driver->capacity)
        {

            pthread_mutex_unlock(&(driver->mutex));
            return DRIVER_REQUEST_FULL;
        }
        if (queue_add(driver->queue, job) == QUEUE_SUCCESS)
        {

            if (driver->list != NULL)
            {
                list_node_t *node;

                node = list_find(driver->list, 2);
                if (node != NULL)
                {
                    *(node->index) = node->data;
                    *(node->opera) = node->op;
                    *(node->select) = (void *)driver;
                    pthread_cond_signal(node->cv);
                }
                else
                {
                    pthread_cond_signal(&(driver->empty));
                }
            }
            else
            {
                pthread_cond_signal(&(driver->empty));
            }
        }
        else
        {
            pthread_mutex_unlock(&(driver->mutex));
            return DRIVER_GEN_ERROR;
        }
    }
    else
    {

        if ((driver->count) >= 0)
        {

            pthread_mutex_unlock(&(driver->mutex));
            return DRIVER_REQUEST_FULL;
        }

        driver->unqueuedJob = job;
        if (driver->unqueuedJob == job)
        {

            driver->count++;
        }
        else
        {
            return DRIVER_GEN_ERROR;
        }

        pthread_cond_signal(&(driver->empty));
    }

    pthread_mutex_unlock(&(driver->mutex));

    return SUCCESS;
}

/*
	driver_non_blocking_handle: Picks up data from the given driver and stores it in the
	function’s input parameter job (Note that it is a double pointer). This is a non-blocking
	call, i.e., the function simply returns if the driver is empty. The return type is enum
	driver_status as defined in driver.h. Return
		o SUCCESS for successful retrieval of job,
		o DRIVER_EMPTY if the driver is empty and nothing was stored in job
		o DRIVER_CLOSED_ERROR when the driver is closed, and
		o DRIVER_GEN_ERROR on encountering any other generic error of any sort. 
*/

enum driver_status driver_non_blocking_handle(driver_t *driver, void **job)
{
    /* IMPLEMENT THIS */

    /*
	int ret;
	if (!driver){
		return DRIVER_GEN_ERROR;
	}
	if (driver->status == DRIVER_CLOSED_ERROR){
		return DRIVER_CLOSED_ERROR;
	}
	while(1){
		// -1 from sem, if the sem is not 0
		ret = sem_trywait(&full); 
		//pthread_mutex_lock(&mutex);// blocking lock
		if((ret == -1) && (errno == EAGAIN)){
			driver->status = DRIVER_REQUEST_EMPTY;
			return DRIVER_REQUEST_EMPTY;
		}else if ((ret == -1) && ((errno == EINTR) || (errno == EINVAL))){
			driver->status = DRIVER_GEN_ERROR;
			return DRIVER_GEN_ERROR;
		}else{
			//  if successful have mutex, then add job
			if (queue_remove(driver->queue, job) == QUEUE_SUCCESS){
				driver->status = SUCCESS;
				//pthread_mutex_unlock(&mutex); // unlock
				sem_post(&empty);
			
				return SUCCESS;
			}else{
				driver->status = DRIVER_GEN_ERROR;
				//pthread_mutex_unlock(&mutex);
				driver->status = DRIVER_GEN_ERROR;
				return DRIVER_GEN_ERROR;
			}
		}
	}
	*/

    pthread_mutex_lock(&(driver->mutex));
    if (driver->driver_closed)
    {
        pthread_mutex_unlock(&(driver->mutex));
        return DRIVER_CLOSED_ERROR;
    }
    if (driver->capacity != 0)
    {
        if (driver->queue->size == 0)
        {

            pthread_mutex_unlock(&(driver->mutex));
            return DRIVER_REQUEST_EMPTY;
        }
        if (queue_remove(driver->queue, job) == QUEUE_SUCCESS)
        {

            if (driver->list != NULL)
            {
                list_node_t *node;
                node = list_find(driver->list, 1);
                if (node != NULL)
                {
                    *(node->index) = node->data;
                    *(node->opera) = node->op;
                    *(node->select) = (void *)driver;
                    pthread_cond_signal(node->cv);
                }
                else
                {
                    pthread_cond_signal(&(driver->full));
                }
            }
            else
            {
                pthread_cond_signal(&(driver->full));
            }
        }
        else
        {
            pthread_mutex_unlock(&(driver->mutex));
            return DRIVER_GEN_ERROR;
        }
    }
    else
    {

        if (driver->count <= 0)
        {

            pthread_mutex_unlock(&(driver->mutex));
            return DRIVER_REQUEST_EMPTY;
        }

        driver->count--;
        pthread_cond_signal(&(driver->full));

        pthread_cond_wait(&(driver->empty), &(driver->mutex));
        *job = driver->unqueuedJob;
        if (driver->unqueuedJob != *job)
        {
            pthread_mutex_unlock(&(driver->mutex));
            return DRIVER_GEN_ERROR;
        }
    }

    pthread_mutex_unlock(&(driver->mutex));
    return SUCCESS;
}

/*
	driver_close: Closes the driver and informs all the blocking schedule/handle/select calls to
	return with DRIVER_CLOSED_ERROR. Once the driver is closed, schedule/handle/select
	operations will cease to function and return 
		o SUCCESS if close is successful,
		o DRIVER_GEN_ERROR in any other error case. 
*/

enum driver_status driver_close(driver_t *driver)
{
    /* IMPLEMENT THIS */
    pthread_mutex_lock(&(driver->mutex));
    if (!(driver->driver_closed))
    {
        driver->driver_closed = 1;
        if (driver->list != NULL)
        {
            list_node_t *temp = driver->list->head;
            while (temp != NULL)
            {

                if (*(temp->index) == 9999)
                {
                    *(temp->index) = temp->data;
                    *(temp->select) = (void *)driver;
                    pthread_cond_signal(temp->cv);
                }

                temp = temp->next;
            }
        }
        if (driver->queue->size == 0)
        {
            pthread_cond_broadcast(&(driver->empty));
        }
        else if (driver->queue->size == driver->capacity)
        {

            pthread_cond_broadcast(&(driver->full));
        }
        pthread_mutex_unlock(&(driver->mutex));
        return SUCCESS;
    }
    pthread_mutex_unlock(&(driver->mutex));
    return DRIVER_GEN_ERROR;
}

/*
	driver_destroy: Free all the memory allocated to the driver. The caller is responsible for
	calling driver_close and waiting for all threads to finish their tasks before calling
	driver_destroy. Return
		o SUCCESS if destroy is successful,
		o DRIVER_DESTROY_ERROR if driver_destroy is called on an open driver, and
		o DRIVER_GEN_ERROR in any other error case.
*/
enum driver_status driver_destroy(driver_t *driver)
{
    /* IMPLEMENT THIS */
    /*
	sem_destroy(&full);
	sem_destroy(&empty);
	pthread_mutex_destroy(&mutex);
	free(driver);
	*/
    if (!(driver->driver_closed))
    {
        pthread_mutex_unlock(&(driver->mutex));
        return DRIVER_DESTROY_ERROR;
    }

    pthread_mutex_lock(&(driver->mutex));
    if (driver->driver_closed)
    {
        if (driver->list != NULL)
        {
            list_destroy(driver->list);
            driver->list = NULL;
        }
        queue_free(driver->queue);
        free(driver);
        pthread_mutex_unlock(&(driver->mutex));
        pthread_mutex_destroy(&(driver->mutex));
        pthread_cond_destroy(&(driver->empty));
        pthread_cond_destroy(&(driver->full));
        return SUCCESS;
    }

    pthread_mutex_unlock(&(driver->mutex));
    return DRIVER_GEN_ERROR;
}

/*
	driver_select: Takes an array of drivers, driver_list, of type select_t and the array length,
	driver_count, as inputs. This API iterates over the provided list and finds the set of possible
	drivers which can be used to invoke the required operation (schedule or handle) specified in
	select_t. If multiple options are available, it selects the first option and performs its
	corresponding action. If no driver is available, the call is blocked and waits until it finds a
	driver which supports its required operation. Once an operation has been successfully
	performed, select should
		o set selected_index to the index of the driver that performed the operation and then
			return SUCCESS.
		o In the event that a driver is closed or encounters an error such as
			DRIVER_GEN_ERROR, you should propagate the error and return the error through
			select. Additionally, set selected_index to the index of the driver that generated the
			error. 
*/
enum driver_status driver_select(select_t *driver_list, size_t driver_count, size_t *selected_index)
{

    pthread_cond_t select_cv = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&mutex);
    size_t i;
    size_t index = 9999;
    int opera = 0;
    void *select = NULL;

    for (i = 0; i < driver_count; i++)
    {

        driver_t *driver = driver_list[i].driver;

        if (driver->driver_closed)
        {
            *selected_index = i;
            if (i > 0)
            {
                size_t x;
                for (x = 0; x < i; x++)
                {
                    list_node_t *node;

                    node = helper(driver_list[x].driver->list, &select_cv);
                    list_remove(driver_list[x].driver->list, node);
                }
            }
            pthread_mutex_unlock(&mutex);
            return DRIVER_CLOSED_ERROR;
        }

        if (queue_current_size(driver->queue) < driver->capacity && driver_list[i].op == SCHDLE)
        {
            *selected_index = i;
            if (queue_add(driver->queue, driver_list[i].job) == QUEUE_SUCCESS)
            {

                if (i > 0)
                {
                    size_t x;
                    for (x = 0; x < i; x++)
                    {
                        list_node_t *node;

                        node = helper(driver_list[x].driver->list, &select_cv);
                        list_remove(driver_list[x].driver->list, node);
                    }
                }
                if (driver->list != NULL)
                {

                    list_node_t *node;

                    node = list_find(driver->list, 2);
                    if (node != NULL)
                    {
                        *(node->index) = node->data;
                        *(node->opera) = node->op;
                        *(node->select) = (void *)driver;

                        pthread_cond_signal(node->cv);
                    }
                    else
                    {
                        pthread_cond_signal(&(driver->empty));
                    }
                }
                else
                {
                    pthread_cond_signal(&(driver->empty));
                }
            }
            else
            {

                pthread_mutex_unlock(&mutex);
                return DRIVER_GEN_ERROR;
            }
            pthread_mutex_unlock(&mutex);
            return SUCCESS;
        }

        if (queue_current_size(driver->queue) != 0 && driver_list[i].op == HANDLE)
        {
            *selected_index = i;
            if (queue_remove(driver->queue, &(driver_list[i].job)) == QUEUE_SUCCESS)
            {

                if (i > 0)
                {
                    size_t x;
                    for (x = 0; x < i; x++)
                    {
                        list_node_t *node;

                        node = helper(driver_list[x].driver->list, &select_cv);
                        list_remove(driver_list[x].driver->list, node);
                    }
                }
                if (driver->list != NULL)
                {

                    list_node_t *node;
                    node = list_find(driver->list, 1);
                    if (node != NULL)
                    {
                        *(node->index) = node->data;
                        *(node->opera) = node->op;
                        *(node->select) = (void *)driver;
                        pthread_cond_signal(node->cv);
                    }
                    else
                    {
                        pthread_cond_signal(&(driver->full));
                    }
                }
                else
                {
                    pthread_cond_signal(&(driver->full));
                }
            }
            else
            {

                pthread_mutex_unlock(&mutex);
                return DRIVER_GEN_ERROR;
            }

            pthread_mutex_unlock(&mutex);

            return SUCCESS;
        }

        if (driver->list == NULL)
        {
            driver->list = list_create();
        }

        if (driver_list[i].op == SCHDLE)
        {

            list_insert(driver->list, i, 1, &(select_cv), &(index), &(opera), &(select));
        }
        else if (driver_list[i].op == HANDLE)
        {

            list_insert(driver->list, i, 2, &(select_cv), &(index), &(opera), &(select));
        }
    }
    while (((opera == 1) && (((driver_t *)select)->queue->size == ((driver_t *)select)->queue->capacity)) || ((opera == 2) && (((driver_t *)select)->queue->size == 0)) || (index == 9999))
    {
        pthread_cond_wait(&(select_cv), &mutex);
        if (((driver_t *)select)->driver_closed)
        {
            *selected_index = index;
            size_t j;
            for (j = 0; j < driver_count; j++)
            {
                list_node_t *node;
                node = helper(driver_list[j].driver->list, &select_cv);
                list_remove(driver_list[j].driver->list, node);
            }
            pthread_mutex_unlock(&mutex);
            return DRIVER_CLOSED_ERROR;
        }
    }

    *selected_index = index;

    if (opera == 1)
    {

        if (queue_add(((driver_t *)select)->queue, driver_list[index].job) == QUEUE_SUCCESS)
        {

            if (((driver_t *)select)->list != NULL)
            {

                list_node_t *node;
                node = list_find(((driver_t *)select)->list, 2);
                if (node != NULL)
                {
                    *(node->index) = node->data;
                    *(node->opera) = node->op;
                    *(node->select) = (void *)select;
                    pthread_cond_signal(node->cv);
                }
                else
                {
                    pthread_cond_signal(&(((driver_t *)select)->empty));
                }
            }
            else
            {
                pthread_cond_signal(&(((driver_t *)select)->empty));
            }
        }
        else
        {

            pthread_mutex_unlock(&mutex);
            return DRIVER_GEN_ERROR;
        }
    }
    else if (opera == 2)
    {

        if (queue_remove(((driver_t *)select)->queue, &(driver_list[index].job)) == QUEUE_SUCCESS)
        {

            if (((driver_t *)select)->list != NULL)
            {

                list_node_t *node;
                node = list_find(((driver_t *)select)->list, 1);
                if (node != NULL)
                {
                    *(node->index) = node->data;
                    *(node->opera) = node->op;
                    *(node->select) = (void *)select;
                    pthread_cond_signal(node->cv);
                }
                else
                {
                    pthread_cond_signal(&(((driver_t *)select)->full));
                }
            }
            else
            {
                pthread_cond_signal(&(((driver_t *)select)->full));
            }
        }
        else
        {
            pthread_mutex_unlock(&mutex);
            return DRIVER_GEN_ERROR;
        }
    }
    size_t j;
    for (j = 0; j < driver_count; j++)
    {
        list_node_t *node;
        node = helper(driver_list[j].driver->list, &select_cv);
        list_remove(driver_list[j].driver->list, node);
    }
    pthread_mutex_unlock(&mutex);

    return SUCCESS;
}

list_node_t *helper(list_t *list, pthread_cond_t *cv)
{

    if (list->count == 0)
    {
        return NULL;
    }
    else
    {
        list_node_t *node;

        node = list->head;
        while (node->cv != cv)
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
