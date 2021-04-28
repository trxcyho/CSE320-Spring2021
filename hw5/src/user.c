#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "server.h"
#include "globals.h"
#include <csapp.h>

//prototypes

struct user {
	char * name;
	int ref_count;
};

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
	new_user -> ref_count = 1;
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
	user ->ref_count = (user -> ref_count) + 1;
	debug("Reference Count increased: %s\n", why);
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
	user -> ref_count = (user-> ref_count) -1;
	debug("Reference coutn decreased: %s\n", why);
	if((user->ref_count) == 0){
		free(user-> name);
		free(user);
		debug("Free user becauser ref count = 0");
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