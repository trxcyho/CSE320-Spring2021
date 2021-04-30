#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "server.h"
#include "globals.h"
#include <csapp.h>

//prototypes
typedef struct user {
	char * name;
	sem_t sem;
	int ref_count;
}USER;

/*
 * Create a new USER with a specified handle.  A private copy is
 * made of the handle that is passed.  The newly created USER has
 * a reference count of one, corresponding to the reference that is
 * returned from this function.
 *
 * @param handle  The handle of the USER.
 * @return  A reference to the newly created USER, if initialization
 * was successful, otherwise NULL.
 */
USER *user_create(char *handle){
	USER *new_user = malloc(sizeof(USER));
	char* copy = malloc(strlen(handle) + 1);//for null terminator
	strcpy(copy, handle);
	new_user -> name = copy;
	//set reference count
	Sem_init(&(new_user->sem), 0, 1);
	new_user -> ref_count = 1;
	debug("Create user [%s]\n", new_user-> name);
	return new_user;
}

/*
 * Increase the reference count on a user by one.
 *
 * @param user  The USER whose reference count is to be increased.
 * @param why  A string describing the reason why the reference count is
 * being increased.  This is used for debugging printout, to help trace
 * the reference counting.
 * @return  The same USER object that was passed as a parameter.
 */
USER *user_ref(USER *user, char *why){
	P(&(user->sem));
	user ->ref_count = (user -> ref_count) + 1;
	debug("Increased Reference Count[%s] (%d -> %d): %s\n", user-> name, (user ->ref_count) -1, user ->ref_count, why);
	V(&(user->sem));
	return user;
}

/*
 * Decrease the reference count on a USER by one.
 * If after decrementing, the reference count has reached zero, then the
 * USER and its contents are freed.
 *
 * @param user  The USER whose reference count is to be decreased.
 * @param why  A string describing the reason why the reference count is
 * being decreased.  This is used for debugging printout, to help trace
 * the reference counting.
 *
 */
void user_unref(USER *user, char *why){
	P(&(user->sem));
	user -> ref_count = (user-> ref_count) -1;
	debug("Decreased Reference Count[%s] (%d -> %d): %s\n", user-> name, (user ->ref_count)+1, user ->ref_count, why);
	V(&(user->sem));
	if((user->ref_count) == 0){
		debug("Free [%s] because ref count = 0", user->name);
		free(user-> name);
		free(user);
		//destroy semtex?
	}
}

/*
 * Get the handle of a user.
 *
 * @param user  The USER that is to be queried.
 * @return the handle of the user.
 */
char *user_get_handle(USER *user){
	return user->name;
}