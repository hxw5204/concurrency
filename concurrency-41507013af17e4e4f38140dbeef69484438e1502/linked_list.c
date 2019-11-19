#include "linked_list.h"

// Create and return a new list
list_t* list_create()
{
	/* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
	list_t* linked_list;
	linked_list = malloc(sizeof(list_t));
	linked_list->head = NULL;
	linked_list->count = 0;

	return linked_list;
}

// Destroy a list
void list_destroy(list_t* list)
{
	/* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */

	list_node_t* temp;

	if(list == NULL){
		return;
	}

	while(list->head!=NULL){
		temp = list->head;
		list->head = list->head->next;
		free(temp);
		
	}

	free(list);
	list = NULL;
	return;

}

// Return the number of elements in the list
size_t list_count(list_t* list)
{
	/* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
	return list->count;
}

// Find the first node in the list with the given data
// Returns NULL if data could not be found
list_node_t* list_find(list_t* list, int op)
{
	/* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
	if(list->count == 0){
		return NULL;
	}
	list_node_t* node;
	node = list->head;

	while(node ->op != op){
		node = node->next;
		if(node == NULL){
			return NULL;
		}
	}

	return node;
}

// Insert a new node in the list with the given data
void list_insert(list_t* list, size_t data, int op, sem_t* list_sem, size_t index, int* opera, void** select)
{
	/* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
	list_node_t* node;
	node = (list_node_t*)malloc(sizeof(list_node_t));

	if(list->count == 0){
		list->head = node;
		list->head->next = NULL;
		list->head->prev = NULL;
		list->head->data = data;
		list->head->op = op;
		list->head->list_sem = list_sem;
		list->head->index = index;
		list->head->opera=opera;
		list->head->select = select;

	}else{

		node->next=NULL;
		list_node_t* temp=list->head;
		while(temp->next != NULL){
			temp = temp->next;
		}

		node->prev = temp;
		temp->next=node;
		node->data=data;
		node->op=op;
		node->list_sem=list_sem;
		node->index=index;
		node->opera=opera;
		node->select=select;
	}

	list->count=(list->count)+1;
}

// Remove a node from the list and free the node resources
void list_remove(list_t* list, list_node_t* node)
{
	/* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
	if(list == NULL){
		return;
	}

	if(node->next != NULL){
		node->next->prev = node->prev;
	}

	if(node->prev != NULL){
		node->prev->next = node->next;
	}

	if(list->head == node){
		list->head = node->next;
	}

	list->count--;
	free(node);
}

// Execute a function for each element in the list
void list_foreach(list_t* list, void (*func)(void* data))
{
	/* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
	
}

list_node_t* node_delete(list_t* list, sem_t* list_sem){
	if(list->count == 0){
		return NULL;
	}

	list_node_t* node = list->head;

	while((node->list_sem != list_sem)){
		node = node->next;
		if(node==NULL){
			return NULL;
		}
	}
	return node;
}

list_node_t* node_index(list_t* list, sem_t* list_sem, int op){
	if(list->count == 0){
		return NULL;
	}

	list_node_t* node = list->head;

	while((node->list_sem != list_sem) && (node->op != op)){
		node = node->next;
		if(node==NULL){
			return NULL;
		}
	}
	return node;
}
