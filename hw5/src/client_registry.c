#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "server.h"
#include "globals.h"
#include "client.h"
#include <csapp.h>


struct client_registry {
	CLIENT *client_array[MAX_CLIENTS];
	int num_clients;
	sem_t sem;
};

/*
 * Initialize a new client registry.
 *
 * @return  the newly initialized client registry, or NULL if initialization
 * fails.
 */
CLIENT_REGISTRY *creg_init(){
	debug ("Initialize client registry");
	CLIENT_REGISTRY *new_creg = malloc(sizeof(CLIENT_REGISTRY));
	if(new_creg == NULL)
		return NULL;
	new_creg -> num_clients = 0;
	sem_init(&(new_creg -> sem), 0, 1);
	return new_creg;
}

/*
 * Finalize a client registry, freeing all associated resources.
 *
 * @param cr  The client registry to be finalized, which must not
 * be referenced again.
 */
void creg_fini(CLIENT_REGISTRY *cr){
	for(int i = 0; i < cr -> num_clients; i++)
		free(cr->client_array[i]);
	free(cr);
	debug("Client Registry freed");
}

/*
 * Register a client connection.
 * If successful, returns a reference to the the newly registered CLIENT,
 * otherwise NULL.  The returned CLIENT has a reference count of two:
 * one for the pointer retained by the registry and one for the pointer
 * returned to the caller.
 *
 * @param cr  The client registry.
 * @param fd  The file descriptor of the connection to the client.
 * @return a reference to the newly registered CLIENT, if registration
 * is successful, otherwise NULL.
 */
CLIENT *creg_register(CLIENT_REGISTRY *cr, int fd){
	//client registry full
	if(cr-> num_clients >= 64)
		return NULL;
	CLIENT *client = client_create(cr, fd);
	client_ref(client, "adding to client registry");
	if(client == NULL)
		return NULL;
	P(&(cr-> sem));
	cr->client_array[cr->num_clients] = client;
	cr -> num_clients = (cr -> num_clients) + 1;
	V(&(cr->sem));
	debug("Register client fd %d (total connected: %d)", fd, cr -> num_clients);
	return client;
}

/*
 * Unregister a CLIENT, removing it from the registry.
 * The file descriptor associated with the client connection is *not*
 * closed by this function; that remains the responsibility of whomever
 * originally obtained it.
 * It is an error if the CLIENT is not among the currently registered
 * clients at the time this function is called.
 * The reference count of the CLIENT is decreased by one to account
 * for the pointer being discarded.  If this results in the number of
 * connected clients reaching zero, then any threads waiting in
 * creg_shutdown_all() are allowed to proceed.
 *
 * @param cr  The client registry.
 * @param client  The CLIENT to be unregistered.
 * @return 0  if unregistration succeeds, otherwise -1.
*/

int creg_unregister(CLIENT_REGISTRY *cr, CLIENT *client){
	P(&(cr->sem));
	int tracker = -1;
	for(int i = 0; i < cr->num_clients; i++){
		if(cr-> client_array[i] == client){
			tracker = i;
			break;
		}
	}
	if(tracker == -1)
		return -1;
	//shift everything down?
	client_unref(cr->client_array[tracker], "client removed from client registry");
	for(int i = tracker; i < cr -> num_clients - 1; i++){
		CLIENT *updated = cr->client_array[i+1];
		cr->client_array[i] = updated;
	}
	cr->num_clients = (cr->num_clients) -1;
	if(cr->num_clients == 0){
		V(&(cr->sem));
		creg_shutdown_all(cr);
	}
	V(&(cr->sem));
	debug("Unregister client fd %d (total connected: %d)", client_get_fd(client), cr-> num_clients);
	return 0;
}

/*
 * Return a list of all currently connected clients.  The result is
 * returned as a malloc'ed array of CLIENT pointers, with a NULL
 * pointer marking the end of the array.  Each CLIENT in the array
 * has had its reference count incremented by one, to account for
 * the pointer stored in the array.  It is the caller's responsibility
 * to decrement the reference count of each of the entries and to
 * free the array when it is no longer needed.
 *
 * @param cr  The registry from which the set of clients is to be
 * obtained.
 * @return the list of clients as a NULL-terminated array.
 */
CLIENT **creg_all_clients(CLIENT_REGISTRY *cr){
	debug("in creg_all_clients");
	P(&cr->sem);
	CLIENT **list_clients = malloc(sizeof(CLIENT*)*(cr->num_clients));
	if(list_clients == NULL)
		return NULL;
	for(int i = 0; i < cr->num_clients; i++){
		client_ref(cr->client_array[i], "reference in creg_all_clients");
		list_clients[i] = cr->client_array[i];
	}
	list_clients[cr-> num_clients] = NULL;
	V(&cr->sem);
	return list_clients;
}

/*
 * Shut down (using shutdown(2)) all the sockets for connections
 * to currently registered clients.  The calling thread will block
 * in this function until all the server threads have recognized
 * the EOF on their connections caused by the socket shutdown and
 * have unregistered the corresponding clients.  This function
 * returns only when the number of registered clients has reached zero.
 * This function may be called more than once, but not concurrently.
 * Calling this function does not finalize the client registry.
 *
 * @param cr  The client registry.
 */
void creg_shutdown_all(CLIENT_REGISTRY *cr){
	debug("in creg_shutdown_all");
	// pthread_mutex_t shutdown_mutex;
	// pthread_mutex_init(&shutdown_mutex, NULL);
	// pthread_mutex_lock(&shutdown_mutex);
	P(&(cr->sem));
	if(cr-> num_clients != 0){
		for(int i = 0; i < cr-> num_clients; i++){
			shutdown(client_get_fd(cr->client_array[i]), SHUT_RDWR);
		}
	}
	V(&(cr->sem));
	// pthread_mutex_unlock(&shutdown_mutex);
}

