#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "debug.h"
#include "server.h"
#include "globals.h"
#include <csapp.h>




void *chla_client_service(void *arg){
	Pthread_detach(pthread_self());
	int fd = *((int *)arg);
	free(arg);
	CHLA_PACKET_HEADER hdr;
	void *payload = NULL;

	debug("Starting client service %d", fd);
	CLIENT *client = creg_register(client_registry, fd);
	while(1){//fix while loop so theres stopping condition
		if(proto_recv_packet(fd, &hdr, &payload) == -1){
			switch(hdr->type){
				case CHLA_LOGIN_PKT:
				//
					break;
				default:
					break;
			}
		}
	}
}