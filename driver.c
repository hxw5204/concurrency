#include "driver.h"

//sem_t sem_driver;
sem_t full;
sem_t empty;

/*
	Creates a new driver with the provided job queue size and returns it to the caller function.
	A 0 size indicates an unqueued driver, whereas a positive size indicates a queued driver.
*/
driver_t* driver_create(size_t size){
	/* IMPLEMENT THIS */

	driver_t* newDriver = (driver_t*)malloc(sizeof(driver_t));
	newDriver->queue = queue_create(size);

	//newDriver->status = SUCCESS;
	//newDriver->count = 0;
	//newDriver->list = NULL;

	pthread_mutex_init(&newDriver->mutex, NULL);
	pthread_cond_init(&newDriver->handle_cv, NULL);
	pthread_cond_init(&newDriver->schedule_cv, NULL);

	//sem_init(&empty, 0, 1);
	//sem_init(&full, 0, 0);
	
	newDriver->unqueueJob = NULL;
	newDriver->driver_closed = 0;

	return newDriver;
}


// Checks if the given driver has space to accommodate the new job and schedule it. This is a blocking call,
// the function only returns on a successful completion of schedule. In case the queue is full, the function
// waits till the queue is available to take in new job. The return type is enum driver_status as defined in griver.h.
// Return
// 		- SUCCESS for successful queuing of job,
// 		- DRIVER_CLOSED_ERROR when the queue is closed
// 		- DRIVER_GEN_ERROR on encountering any other generic error of any sort.
enum driver_status driver_schedule(driver_t *driver, void* job) {
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
		sem_wait(&empty); 
		pthread_mutex_lock(&mutex);// blocking lock
		
		//  if successful have mutex, then add job
		if (queue_add(driver->queue, job) == QUEUE_SUCCESS){
			pthread_mutex_unlock(&mutex); // unlock
			sem_post(&full);
			driver->status = SUCCESS;
			return SUCCESS;
		}else{
			pthread_mutex_unlock(&mutex);
			driver->status = DRIVER_GEN_ERROR;
			return DRIVER_GEN_ERROR;
		}
	}
	*/

	pthread_mutex_lock(&driver->mutex);

	if (!driver){
		return DRIVER_GEN_ERROR;
	}

	if(driver->driver_closed){
		pthread_mutex_unlock(&driver->mutex);
		return DRIVER_CLOSED_ERROR;
	}
	while(queue_current_size(driver->queue) == queue_capacity(driver->queue)){
		pthread_cond_wait(&driver->schedule_cv, &driver->mutex);

		if(driver->driver_closed){
			pthread_mutex_unlock(&driver->mutex);
			return DRIVER_CLOSED_ERROR;
		}
	}

	queue_add(driver->queue, job);
	pthread_cond_signal(&driver->handle_cv);
	pthread_mutex_unlock(&driver->mutex);

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
enum driver_status driver_handle(driver_t *driver, void **job) {
	/* IMPLEMENT THIS */

	
	if (!driver){
		return DRIVER_GEN_ERROR;
	}
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

	pthread_mutex_lock(&driver->mutex);
	if(driver -> driver_closed){
		pthread_mutex_unlock(&driver->mutex);
		return DRIVER_CLOSED_ERROR;
	}

	while(queue_current_size(driver->queue) == 0){
		pthread_cond_wait(&driver->handle_cv, &driver->mutex);
	}

	queue_remove(driver->queue, job);
	pthread_cond_signal(&driver->schedule_cv);
	pthread_mutex_unlock(&driver->mutex);

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
enum driver_status driver_non_blocking_schedule(driver_t *driver, void* job) {
	/* IMPLEMENT THIS */

	int ret;
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
	pthread_mutex_lock(&driver ->mutex);

	if(!driver){
		pthread_mutex_unlock(&driver->mutex);
		return DRIVER_DESTROY_ERROR;
	}

	if(driver->driver_closed){
		pthread_mutex_unlock(&driver->mutex);
		return DRIVER_CLOSED_ERROR;
	}

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
enum driver_status driver_non_blocking_handle(driver_t *driver, void **job) {
	/* IMPLEMENT THIS */
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
}


/*
	driver_close: Closes the driver and informs all the blocking schedule/handle/select calls to
	return with DRIVER_CLOSED_ERROR. Once the driver is closed, schedule/handle/select
	operations will cease to function and return 
		o SUCCESS if close is successful,
		o DRIVER_GEN_ERROR in any other error case. 
*/
enum driver_status driver_close(driver_t *driver) {
	/* IMPLEMENT THIS */

}

/*
	driver_destroy: Free all the memory allocated to the driver. The caller is responsible for
	calling driver_close and waiting for all threads to finish their tasks before calling
	driver_destroy. Return
		o SUCCESS if destroy is successful,
		o DRIVER_DESTROY_ERROR if driver_destroy is called on an open driver, and
		o DRIVER_GEN_ERROR in any other error case.
*/
enum driver_status driver_destroy(driver_t *driver) {
	/* IMPLEMENT THIS */
	sem_destroy(&full);
	sem_destroy(&empty);
	pthread_mutex_destroy(&mutex);
	free(driver);

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
enum driver_status driver_select(select_t *driver_list, size_t driver_count, size_t* selected_index) {
	/* IMPLEMENT THIS */
}

