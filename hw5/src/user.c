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


USER *user_ref(USER *user, char *why){
	P(&(user->sem));
	user ->ref_count = (user -> ref_count) + 1;
	debug("Increased Reference Count[%s] (%d -> %d): %s\n", user-> name, (user ->ref_count) -1, user ->ref_count, why);
	V(&(user->sem));
	return user;
}


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


char *user_get_handle(USER *user){
	return user->name;
}