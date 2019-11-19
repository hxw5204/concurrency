#include "driver.h"


driver_t* driver_create(size_t size){
	/* IMPLEMENT THIS */
	driver_t* driver = malloc(sizeof(driver_t));

	driver->driver_capacity = size;

	driver->queue=queue_create(size);

	driver->driver_closed = 0;

	pthread_mutex_init(&driver->mutex, NULL);//mutex lock

	sem_init(&driver->empty, 0, 0);
	sem_init(&driver->full, 0, (unsigned int)size);

	driver->list = list_create();
	driver->count = 0;

	driver->unqueuedJob = NULL;

	return driver;
}

enum driver_status driver_schedule(driver_t *driver, void* job) {
	/* IMPLEMENT THIS */

	/*
	pthread_mutex_lock(&driver->mutex); //lock the queued
	


	if((driver->driver_closed)){
		//sem_post(&driver->full);
		pthread_mutex_unlock(&driver->mutex);

		return DRIVER_CLOSED_ERROR;

	}

	sem_wait(&driver->full); //wait for available space

	if(queue_add(driver->queue, job) == DRIVER_GEN_ERROR){
		pthread_mutex_unlock(&driver->mutex); //unlock
		return DRIVER_GEN_ERROR;
	}else{

		if(driver->list->head != NULL){
			list_node_t* node;
			node = driver->list->head;


			for(int i = 0; i<driver->list->count; i++){
				sem_post(node->list_sem);
				node = node->next;
			}
		}

		sem_post(&driver->empty);
		pthread_mutex_unlock(&driver->mutex);

		return SUCCESS;

	}
	*/
	pthread_mutex_lock(&driver->mutex); //lock the queued
	


	if((driver->driver_closed)){
		//sem_post(&driver->full);
		pthread_mutex_unlock(&driver->mutex);

		return DRIVER_CLOSED_ERROR;

	}

	if(driver->driver_capacity != 0){
		while(driver->queue->size == driver->driver_capacity){

			sem_wait(&driver->full);

			if(driver->driver_closed){
				pthread_mutex_unlock(&driver->mutex);
				return DRIVER_CLOSED_ERROR;
			}
		}

		if(queue_add(driver->queue,job) == QUEUE_SUCCESS){

			if(driver->list != NULL){
				list_node_t* node = list_find(driver->list, 2);

				if(node != NULL){
					*(node->index) = node->data;
					*(node->opera) = node->op;
					*(node->select) = (void *)driver;
					sem_post(node->list_sem);
				}else{

					sem_post(&driver->empty);
				}
			}else{
				sem_post(&driver->empty);
			}
		}else{
			pthread_mutex_unlock(&driver->mutex);
			return DRIVER_GEN_ERROR;
		}
	}else{

		//unqueued
		int i = 0;

		if(driver->count >= 0){
			i = 1;
			driver->count=(driver->count) +1;
			sem_wait(&(driver->full));
		}

		if(driver->driver_closed){
			pthread_mutex_unlock(&driver->mutex);
			return DRIVER_CLOSED_ERROR;
		}

		if(i==0){
			driver->count = driver->count +1;
		}

		driver->unqueuedJob = job;
		if(driver->unqueuedJob != job){
			pthread_mutex_unlock(&driver->mutex);
			return DRIVER_GEN_ERROR;
		}
		sem_post(&driver->empty);


	}
	pthread_mutex_unlock(&driver->mutex);
	return SUCCESS;
}

enum driver_status driver_handle(driver_t *driver, void **job) {
	/* IMPLEMENT THIS */

	/*
	pthread_mutex_lock(&driver->mutex);
	

	if((driver->driver_closed)){
		
		pthread_mutex_unlock(&driver->mutex);

		return DRIVER_CLOSED_ERROR;

	}

	sem_wait(&driver->empty);
	//sem_post(&driver->empty);
		
	if(queue_remove(driver->queue, job) == QUEUE_ERROR){
		pthread_mutex_unlock(&driver->mutex);
		return DRIVER_GEN_ERROR;
	}else{
		if(driver->list->head != NULL){

			list_node_t* node = driver->list->head;

			for (int i = 0; i< driver->list->count;i++){
				sem_post(node->list_sem);
				node = node->next;
			}
		}

		sem_post(&driver->full);
		pthread_mutex_unlock(&driver->mutex);

		return SUCCESS;
	}
	*/

	pthread_mutex_lock(&driver->mutex);
	

	if((driver->driver_closed)){
		
		pthread_mutex_unlock(&driver->mutex);

		return DRIVER_CLOSED_ERROR;

	}

	if(driver->driver_capacity != 0){
		
		while(driver->queue->size == 0){
			sem_wait(&driver->empty);
			if(driver->driver_closed){
				pthread_mutex_unlock(&driver->mutex);
				return DRIVER_CLOSED_ERROR;
			}
		}

		if(queue_remove(driver->queue,job) == QUEUE_SUCCESS){

			if(driver->list != NULL){
				list_node_t* node = list_find(driver->list, 1);

				if(node != NULL){
					*(node->index) = node->data;
					*(node->opera) = node->op;
					*(node->select) = (void*)driver;
					sem_post(node->list_sem);
				}else{
					sem_post(&driver->full);
				}
			}else{
				sem_post(&driver->full);
			}
		}else{
			pthread_mutex_unlock(&driver->mutex);
			return DRIVER_GEN_ERROR;
		}

	}else{

		//Unqueued
		driver->count = (driver->count)-1;
		sem_post(&driver->full);

		if(1){
			sem_wait(&driver->empty);
		}

		if(driver->driver_closed){
			pthread_mutex_unlock(&driver->mutex);
			return DRIVER_CLOSED_ERROR;
		}

		*job = driver->unqueuedJob;
		if(*job == driver->unqueuedJob){
			driver->unqueuedJob = NULL;
		}else{
			pthread_mutex_unlock(&driver->mutex);
			return DRIVER_GEN_ERROR;
		}
	}
	pthread_mutex_unlock(&driver->mutex);
	return SUCCESS;

}

enum driver_status driver_non_blocking_schedule(driver_t *driver, void* job) {
	/* IMPLEMENT THIS */


	/*
	pthread_mutex_lock(&driver->mutex);

	if((driver->driver_closed)){

		pthread_mutex_unlock(&driver->mutex);
		return DRIVER_CLOSED_ERROR;

	}else{

		if(sem_trywait(&driver->full) != 0){
			pthread_mutex_unlock(&driver->mutex);
			return DRIVER_REQUEST_FULL;
		}else{

			if(queue_add(driver->queue, job) == QUEUE_ERROR){

				pthread_mutex_unlock(&driver->mutex);
				return DRIVER_GEN_ERROR;

			}else{

				sem_post(&driver->empty);

				if(driver->list->head != NULL){

					list_node_t* node;
					node = driver->list->head;

					for(int i = 0; i < driver->list->count; i++){
						sem_post(node->list_sem);
						node = node->next;
					}

				}

				pthread_mutex_unlock(&driver->mutex);
				return SUCCESS;

			}

		}

	}
	*/

	pthread_mutex_lock(&driver->mutex);

	if((driver->driver_closed)){

		pthread_mutex_unlock(&driver->mutex);
		return DRIVER_CLOSED_ERROR;

	}

	if(driver->driver_capacity != 0){
		if(driver->queue->size == driver->driver_capacity){
			pthread_mutex_unlock(&driver->mutex);
			return DRIVER_REQUEST_FULL;
		}

		if(queue_add(driver->queue, job)==QUEUE_SUCCESS){
			
			if(driver->list != NULL){
				list_node_t* node = list_find(driver->list, 2);
				if(find != NULL){
					*(node->index) = node->data;
					*(node->opera) = node->op;
					*(node->select) = (void *)driver;
					sem_post(node->list_sem);
				}else{
					sem_post(&driver->empty);
				}
			}else{
				sem_post(&driver->empty);
			}
		}else{
			pthread_mutex_unlock(&driver->mutex);
		}
	}else{

		//Unqueued
		if(driver->count >= 0){
			pthread_mutex_unlock(&driver->mutex);
			return DRIVER_REQUEST_FULL;
		}

		driver->unqueuedJob=job;

		if(driver->unqueuedJob == job){
			driver->count = driver->count + 1;
		}else{
			pthread_mutex_unlock(&driver->mutex);
			return DRIVER_GEN_ERROR;
		}
		sem_post(&driver->empty);
	}

	pthread_mutex_unlock(&driver->mutex);
	return SUCCESS;

}

enum driver_status driver_non_blocking_handle(driver_t *driver, void **job) {
	/* IMPLEMENT THIS */


	/*
	pthread_mutex_lock(&driver->mutex);

	if((driver->driver_closed)){
		pthread_mutex_unlock(&driver->mutex);
		return DRIVER_CLOSED_ERROR;
	}else{

		if(sem_trywait(&driver->empty) != 0){

			pthread_mutex_unlock(&driver->mutex);
			return DRIVER_REQUEST_EMPTY;

		}else{

			if(queue_remove(driver->queue, job) == QUEUE_ERROR){

				pthread_mutex_unlock(&driver->mutex);
				return DRIVER_GEN_ERROR;

			}else{

				sem_post(&driver->full);

				if(driver->list->head != NULL){

					list_node_t* node = driver->list->head;

					for(int i = 0; i < driver->list->count; i++){

						sem_post(node->list_sem);
						node = node->next;

					}

				}

				pthread_mutex_unlock(&driver->mutex);
				return SUCCESS;

			}
		}

	}
	*/

	pthread_mutex_lock(&driver->mutex);

	if((driver->driver_closed)){
		pthread_mutex_unlock(&driver->mutex);
		return DRIVER_CLOSED_ERROR;
	}

	if(driver->driver_capacity != 0){

		if(driver->queue->size == 0){
			pthread_mutex_unlock(&driver->mutex);
			return DRIVER_REQUEST_EMPTY;
		}

		if(queue_remove(driver->queue, job) == QUEUE_SUCCESS){

			if(driver->list != NULL){
				list_node_t* node = list_find(driver->list, 1);
				if(node != NULL){
					*(node->index) = node->data;
					*(node->opera) = node->op;
					*(node->select) = (void *)driver;
					sem_post(node->list_sem);
				}else{
					sem_post(&driver->full);

				}
			}else{
				sem_post(&driver->full);
			}
		}else{
			pthread_mutex_unlock(&driver->mutex);
			return DRIVER_GEN_ERROR;
		}

	}else{
		//unqueuued
		if(driver->count <= 0){
			pthread_mutex_unlock(&driver->mutex);
			return DRIVER_REQUEST_EMPTY;
		}

		driver->count = driver->count -1;
		sem_post(&driver->full);
		sem_wait(&driver->empty);
		*job = driver->unqueuedJob;

		if(driver->unqueuedJob != *job){
			pthread_mutex_unlock(&driver->mutex);
			return DRIVER_GEN_ERROR;
		}
	}

	pthread_mutex_unlock(&driver->mutex);
	return SUCCESS;

}

enum driver_status driver_close(driver_t *driver) {
	/* IMPLEMENT THIS */

	/*
	pthread_mutex_lock(&driver->mutex);

	if(driver->driver_closed){

		pthread_mutex_unlock(&driver->mutex);
		return DRIVER_GEN_ERROR;

	}else{

		driver->driver_closed = 1;

		sem_post(&driver->empty);
		sem_post(&driver->full);
		
		if(driver->list->head != NULL){

			list_node_t* node = driver->list->head;

			for (int i = 0; i < driver->list->count; i++){
				sem_post(node->list_sem);
				node = node->next;

			}
		}

		pthread_mutex_unlock(&driver->mutex);

		return SUCCESS;
	}
	*/

	pthread_mutex_lock(&driver->mutex);

	if(!(driver->driver_closed)) {
		
		if(driver->list != NULL){
			driver->driver_closed = 1;
			list_node_t* node = driver->list->head;
			while(node != NULL){
				if(*(node->index) == 9999){
					*(node->index) = node->data;
					*(node->select) = (void *)driver;
					sem_post(node->list_sem);
				}

				node = node->next;
			}
		}

		if(driver->queue->size == 0){

			sem_post(&driver->empty);

		}else if(driver->queue->size == driver->driver_capacity){

			sem_post(&driver->full);

		}
		pthread_mutex_unlock(&driver->mutex);
		return SUCCESS;
	}
	pthread_mutex_unlock(&driver->mutex);
	return DRIVER_GEN_ERROR;

}

enum driver_status driver_destroy(driver_t *driver) {
	/* IMPLEMENT THIS */

	pthread_mutex_lock(&driver->mutex);
	if(!driver->driver_closed){
		pthread_mutex_unlock(&driver->mutex);
		return DRIVER_DESTROY_ERROR;

	}else if(driver->driver_closed){

		if(driver->list != NULL){
			list_destroy(driver->list);
			driver->list=NULL;
		}

		queue_free(driver -> queue);

		sem_destroy(&driver->empty);
		sem_destroy(&driver->full);

		pthread_mutex_destroy(&driver->mutex);

		free(driver);
		pthread_mutex_unlock(&driver->mutex);
		return SUCCESS;
	}
	else{
		pthread_mutex_unlock(&driver->mutex);
		return DRIVER_GEN_ERROR;
	}
}

enum driver_status driver_select(select_t *driver_list, size_t driver_count, size_t* selected_index) {
	/* IMPLEMENT THIS */

	/*
	sem_t select_sem;
	sem_init(&select_sem, 0 ,0);

	for(int i = 0; i < driver_count; i++){

		pthread_mutex_lock(&driver_list[i].driver->mutex);
		list_insert(driver_list[i].driver->list, &select_sem);
		pthread_mutex_unlock(&driver_list[i].driver->mutex);

	}

	while(1){

		for (int i = 0; i < driver_count; i++){

			// schedule
			if(driver_list[i].op == SCHDLE){

				enum driver_status status = driver_non_blocking_schedule(driver_list[i].driver, driver_list[i].job);

				if(status != DRIVER_REQUEST_FULL){

					*selected_index = (unsigned long)i;
					list_node_t* node;

					for(int j = 0; j< driver_count; j++){

						pthread_mutex_lock(&driver_list[j].driver->mutex);
						node = list_find(driver_list[j].driver->list, &select_sem);

						if(node){
							list_remove(driver_list[j].driver->list, node);
						}

						pthread_mutex_unlock(&driver_list[j].driver->mutex);
					}
					return status;
				}
			}

			// handle
			if(driver_list[i].op == HANDLE){

				enum driver_status status = driver_non_blocking_handle(driver_list[i].driver, &driver_list[i].job);

				if(status != DRIVER_REQUEST_EMPTY){

					*selected_index = (unsigned long) i;
					list_node_t* node;

					for(int j = 0; j< driver_count; j++){

						pthread_mutex_lock(&driver_list[j].driver->mutex);

						node = list_find(driver_list[j].driver->list, &select_sem);

						if(node){
							list_remove(driver_list[j].driver->list, node);
						}

						pthread_mutex_unlock(&driver_list[j].driver->mutex);
					}
					return status;
				}
			}
		}

		sem_wait(&select_sem);
	}
	return SUCCESS;
	*/

	pthread_mutex_lock(&driver->mutex);
	sem_t select_sem;
	sem_init(&select_sem, 0 ,0);
	size_t i;
	size_t index=9999;
	int opera = 0;
	void* selct=NULL;

	for(i=0; i<driver_count;i++){
		driver_t* driver = driver_list[i].driver;

		if(driver->driver_closed){
			*selected_index=i;
			if(i>0){
				size_t x;
				for(x=0;x<i;x++){
					list_node_t* find_del=node_delete(driver_list[x].driver->list,&select_sem);
					list_remove(driver_list[x].driver->list,find_del);
				}
			}
			pthread_mutex_unlock(&drive->mutex);
			return DRIVER_CLOSED_ERROR;
		}

		if(queue_current_size(driver->queue) < queue_capacity(driver->queue) && driver_list[i].op==SCHDLE){
			*selected_index=i;
			if(queue_add(driver->queue,driver_list[i].job) == QUEUE_SUCCESS){
				if(i>0){
					size_t x;
					for (x=0;x<i;x++){
						list_node_t* find = node_delete(driver_list[x].driver->list, &select_sem);
						list_remove(driver_list[x].driver->lsit, find);
					}
				}

				if(driver->list != NULL){
					list_node_t* node = list_find(driver->list, 2);
					if(node != NULL){
						*(node->index) = node->data;
						*(node->opera) = node->op;
						*(node->select) = (void *)driver;

						sem_post(node->list_sem);

					}else{
						sem_post(&driver->empty);
					}
				}else{
					pthread_mutex_unlock(&driver->mutex);
					return DRIVER_GEN_ERROR;
				}
				pthread_mutex_unlock(&driver->mutex);
				return SUCCESS;
			}

			// handle
			if(queue_current_size(driver->queue) !=0 && driver_list[i].op==HANDLE){
				*selected_index=i;
				if(queue_remove(driver->queue, &(driver_list[i].job)) == QUEUE_SUCCESS){

					if(i>0){
						size_t x;
						for(x=0;x<i;x++){
							list_node_t* find = node_delete(driver_list[x].driver->list, &select_sem);
							list_remove(driver_list[x].driver->list,find);
						}
					}

					if(driver->list != NULL){
						list_node_t* node = list_find(driver->list,1);
						if(node != NULL){
							*(node->index) = node->data;
							*(node->opera) = node->op;
							*(node->select) = (void *)driver;
							sem_post(node->list_sem);
						}else{
							sem_post(&driver->full);
						}
					}else{
						sem_post(&driver->full);
					}

				}else{
					pthread_mutex_unlock(&driver->mutex);
					return DRIVER_GEN_ERROR;
				}
				pthread_mutex_unlock(&driver->mutex);
				return SUCCESS;
			}

			if(driver->list==NULL){
				driver->list = list_create();
			}

			if(driver_list[i].op == SCHDLE){
				list_insert(driver->list,i,1,&(select_sem), &index,&opera,&select);
			}else if(driver_list[i].op == HANDLE){

				list_insert(driver->list,i,2,&select_sem, &index, &opera, &select);

			}

		}

		while(((opera==1)&&(((driver_t*)select)->queue->size==((driver_t*)selct)->queue->capacity))||((opera==2)&&(((driver_t *)select)->queue->size==0))||(index==9999)){
			sem_wait(&select_sem);
			if(((driver_t *)select)->driver_closed){
				*selected_index=index;
				size_t j;
				for(j=0;j<driver_count;j++){
					list_node_t* find = node_delete(driver_list[j].driver->list, &select_sem);
					list_remove(driver_list[j].driver->list,find);
				}
				pthread_mutex_unlock(&driver->mutex);
				return DRIVER_CLOSED_ERROR;
			}
		}


	*selected_index=index;

	if(opera==1){
		if(queue_add(((driver_t *)select)->queue,driver_list[index].job)==QUEUE_SUCCESS){
			if(((driver_t *)select)->list != NULL){
				list_node_t* node = list_find(((driver_t *)select)->list, 2);
				if(node != NULL){
					*(node->index) = node->data;
					*(node->opera) = node->op;
					*(node->select) = (void *)select;
					sem_post(node->list_sem);
				}else{
					sem_post(&(((driver_t *)select)->empty));
				}
			}else{
				sem_post(&(((driver_t *)select)->empty));
			}
		}else{
			pthread_mutex_unlock(&driver->mutex);
			return DRIVER_GEN_ERROR;
		}
	}else if(opera == 2){
		if(queue_remove(((driver_t *)select)->queue, &(driver_list[index].job)) == QUEUE_SUCCESS){
			if(((driver_t *)select)->list != NULL){
				list_node_t* node = list_find(((driver_t *)select)->list,1);
				if(node != NULL){
					*(node->index) = node->data;
					*(node->opera) = node->op;
					*(node->select) = (void *)select;
					sem_post(node->list_sem);
				}else{
					sem_post(&(((driver_t *)select)->full));
				}
			}else{
				sem_post(&(((driver_t *)select)->full));
			}
		}else{
			pthread_mutex_unlock(&driver->mutex);
			return DRIVER_GEN_ERROR;
		}
	}

	size_t j;
	for(j = 0; j<driver_count;j++){
		list_node_t* node = node_delete(driver_list[j].driver->list, &select_sem);
		list_remove(driver_list[j],driver->list,node);
	}
	pthread_mutex_unlock(&driver->mutex);
	return SUCCESS;


}
