#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "server.h"
#include "globals.h"
#include "user.h"
#include <csapp.h>

typedef struct node{
	struct node *next;
	USER *user;
} node;

struct user_registry {
	node *head;
};



USER_REGISTRY *ureg_init(void){
	debug("Initialize User Registry");
	USER_REGISTRY *new_user_reg = malloc(sizeof(user_registry));
	return new_user_reg;
}


void ureg_fini(USER_REGISTRY *ureg){
	if(ureg -> head != NULL){
		node *head = ureg -> head;
		node *current;
		while(head -> next != NULL){
			current = head;
			head = head -> next;
			free(current);
		}
		free(head);
	}
	free(ureg);
}


USER *ureg_register(USER_REGISTRY *ureg, char *handle){
	//loop through and see if that handle exists
	debug("Register user %s\n", handle);
	sem_t mutex;
	sem_init(&mutex, 0, 1);
	//no nodes
	if(ureg -> head == NULL){
		debug("User with handle '%s' does not exist yet", handle);
		P(&mutex);
		USER *new_user = user_create(handle);
		node *new_node = malloc(sizeof(node));
		if(new_user == NULL || new_node == NULL)
			return NULL;
		new_node -> user = new_user;
		new_node -> next = NULL;
		ureg -> head = new_node;
		//increase ref count of new user
		user_ref(new_user, "for reference being retained by user registry");
		V(&mutex);
		return new_user;
	}
	node *current = ureg -> head;
	while(current -> next != NULL){
		if(strcmp(user_get_handle(current->user), handle) == 0){
			return current-> user;
		}
		current = current -> next;
	}
	//check tail
	if(strcmp(user_get_handle(current->user), handle) == 0)
		return current -> user;


	//user handle doesn't exist
	debug("User with handle '%s' does not exist yet", handle);
	USER *new_user = user_create(handle);
	node *new_node = malloc(sizeof(node));
	if(new_user == NULL || new_node == NULL)
		return NULL;
	P(&mutex);
	new_node -> user = new_user;
	new_node -> next = NULL;
	current -> next = new_node;
	//increase ref count of new user
	user_ref(new_user, "for reference being retained by user registry");
	V(&mutex);
	return new_user;
}

void ureg_unregister(USER_REGISTRY *ureg, char *handle){
	sem_t mutex;
	sem_init(&mutex, 0, 1);

	node *current = ureg -> head;
	node *links;
	node *freenode;
	//head
	if(strcmp(user_get_handle(current->user), handle) == 0){
		P(&mutex);
		user_unref(current -> user, "for unregistering user");
		links = current -> next;
		ureg -> head = links;
		free(current);//free node
		V(&mutex);
		return;
	}
	//middle
	while(current -> next != NULL){
		if(strcmp(user_get_handle(current->user), handle) == 0){
			P(&mutex);
			user_unref(current -> user, "for unregistering user");
			freenode = current;
			current = current -> next;
			free(freenode);
			links -> next = current;
			V(&mutex);
			return;
		}
		links = current;
		current = current -> next;
	}
	//tail
	if(strcmp(user_get_handle(current->user), handle) == 0){
		P(&mutex);
		user_unref(current -> user, "for unregistering user");
		free(current);
		links-> next = NULL;
		V(&mutex);
		return;
	}
}
