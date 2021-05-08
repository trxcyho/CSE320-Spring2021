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
	sem_t shutdown;
};


CLIENT_REGISTRY *creg_init(){
	debug ("Initialize client registry");
	CLIENT_REGISTRY *new_creg = malloc(sizeof(CLIENT_REGISTRY));
	if(new_creg == NULL)
		return NULL;
	new_creg -> num_clients = 0;
	sem_init(&(new_creg -> sem), 0, 1);
	sem_init(&(new_creg -> shutdown), 0, 1);
	return new_creg;
}

void creg_fini(CLIENT_REGISTRY *cr){
	for(int i = 0; i < cr -> num_clients; i++){
		// client_unref(cr->client_array[i], "finalizing client reg");
		free(cr->client_array[i]);
	}
	free(cr);
	debug("Client Registry freed");
}


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
	debug("Unregister client fd %d (total connected: %d)", client_get_fd(client), cr-> num_clients);
	client_unref(cr->client_array[tracker], "client removed from client registry");
	for(int i = tracker; i < cr -> num_clients - 1; i++){
		CLIENT *updated = cr->client_array[i+1];
		cr->client_array[i] = updated;
	}
	cr->num_clients = (cr->num_clients) -1;
	if(cr->num_clients == 0){
		V(&(cr->shutdown));
		creg_shutdown_all(cr);
	}
	V(&(cr->sem));
	// debug("Unregister client fd %d (total connected: %d)", client_get_fd(client), cr-> num_clients);
	return 0;
}


CLIENT **creg_all_clients(CLIENT_REGISTRY *cr){
	debug("in creg_all_clients");
	P(&cr->sem);
	CLIENT **list_clients = malloc(sizeof(CLIENT*)*(cr->num_clients + 1));
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


void creg_shutdown_all(CLIENT_REGISTRY *cr){
	debug("in creg_shutdown_all");
	// pthread_mutex_t shutdown_mutex;
	// pthread_mutex_init(&shutdown_mutex, NULL);
	// pthread_mutex_lock(&shutdown_mutex);
	if(cr-> num_clients != 0){
		for(int i = 0; i < cr-> num_clients; i++){
			shutdown(client_get_fd(cr->client_array[i]), SHUT_RDWR);
		}
	}
	while(cr-> num_clients != 0){
		P(&(cr->shutdown));
	}
	return;
	// pthread_mutex_unlock(&shutdown_mutex);
}
