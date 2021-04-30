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


/*
 * Initialize a new user registry.
 *
 * @return the newly initialized USER_REGISTRY, or NULL if initialization
 * fails.
 */
USER_REGISTRY *ureg_init(void){
	debug("Initialize User Registry");
	USER_REGISTRY *new_user_reg = malloc(sizeof(user_registry));
	return new_user_reg;
}

/*
 * Finalize a user registry, freeing all associated resources.
 *
 * @param cr  The USER_REGISTRY to be finalized, which must not
 * be referenced again.
 */
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

/*
 * Register a user with a specified handle.  If there is already
 * a user registered under that handle, then the existing registered
 * user is returned, otherwise a new user is created.
 * If an existing user is returned, then its reference count is incremented
 * to reflect the new reference that is being exported from the registry.
 * If a new user is created, then the returned user has reference count
 * equal to two; one corresponding to the reference that is held by
 * the user registry and the other to the pointer that is returned.
 *
 * @param ureg  The user registry into which to register the user.
 * @param handle  The user's handle, which is copied by this function.
 * @return A pointer to a USER object, in case of success, otherwise NULL.
 *
 */
USER *ureg_register(USER_REGISTRY *ureg, char *handle){
	//loop through and see if that handle exists
	debug("Register user %s\n", handle);
	//no nodes
	if(ureg -> head == NULL){
		debug("User with handle '%s' does not exist yet", handle);
		USER *new_user = user_create(handle);
		node *new_node = malloc(sizeof(node));
		if(new_user == NULL || new_node == NULL)
			return NULL;
		new_node -> user = new_user;
		new_node -> next = NULL;
		ureg -> head = new_node;
		//increase ref count of new user
		user_ref(new_user, "for reference being retained by user registry");
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
	new_node -> user = new_user;
	new_node -> next = NULL;
	current -> next = new_node;
	//increase ref count of new user
	user_ref(new_user, "for reference being retained by user registry");
	return new_user;

}

/*
 * Unregister a handle, removing it and the associated USER object from
 * the registry.  The reference count on the USER object is decreased by one.
 * If the handle was not previously registered, nothing is done by this
 * function.
 *
 * @param ureg  The user registry from which to unregister the user.
 * @param handle  The user's handle.
 */
void ureg_unregister(USER_REGISTRY *ureg, char *handle){
	node *current = ureg -> head;
	node *links;
	node *freenode;
	//head
	if(strcmp(user_get_handle(current->user), handle) == 0){
		user_unref(current -> user, "for unregistering user");
		links = current -> next;
		ureg -> head = links;
		free(current);//free node
		return;
	}
	//middle
	while(current -> next != NULL){
		if(strcmp(user_get_handle(current->user), handle) == 0){
			user_unref(current -> user, "for unregistering user");
			freenode = current;
			current = current -> next;
			free(freenode);
			links -> next = current;
			return;
		}
		links = current;
		current = current -> next;
	}
	//tail
	if(strcmp(user_get_handle(current->user), handle) == 0){
		user_unref(current -> user, "for unregistering user");
		free(current);
		links-> next = NULL;
		return;
	}
}
