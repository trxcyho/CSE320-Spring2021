#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "server.h"
#include "globals.h"
#include <csapp.h>

//define prototypes
char *typetostring(CHLA_PACKET_HEADER *hdr);
/*
 * Send a packet with a specified header and payload.
 *   fd - file descriptor on which packet is to be sent
 *   hdr - the packet header, with multi-byte fields in host byte order
 *   payload - pointer to packet payload, or NULL, if none.
 *
 * Multi-byte fields in the header are assumed to be in network byte order.
 *
 * On success, 0 is returned.
 * On error, -1 is returned and errno is set.
 */
int proto_send_packet(int fd, CHLA_PACKET_HEADER *hdr, void *payload){
	//payload can be NULL (login/logout)
	// debug("sending packet\n");
	uint32_t length = htonl(hdr -> payload_length);

	if(length == 0)
		debug("type= %s, payload_length= %d, msgid= %d\n", typetostring(hdr), length, htonl(hdr -> msgid));
	else
		debug("type= %s, payload_length= %d, msgid= %d, payload:[%s]\n", typetostring(hdr), length, htonl(hdr -> msgid), (char *)payload);

	//write header
	int bytes_read = 0;
	int amount;
	while(bytes_read < sizeof(CHLA_PACKET_HEADER)){
		amount = rio_writen(fd, (hdr + bytes_read), sizeof(CHLA_PACKET_HEADER));
		if(amount <= 0)
			return -1; //rio_writen sets errno
		bytes_read += amount;
	}

	//write payload
	bytes_read = 0;
	if(length > 0){
		while(bytes_read < length){
			amount = rio_writen(fd, (payload+ bytes_read), length);
			if(amount <= 0)
				return -1;
			bytes_read+= amount;
		}
	}
	// debug("done sending packet\n");
	return 0;
}

/*
 * Receive a packet, blocking until one is available.
 *  fd - file descriptor from which packet is to be received
 *  hdr - pointer to caller-supplied storage for packet header
 *  payload - variable into which to store payload pointer
 *
 * The returned header has its multi-byte fields in network byte order.
 *
 * If the returned payload pointer is non-NULL, then the caller
 * is responsible for freeing the storage.
 *
 * On success, 0 is returned.
 * On error, -1 is returned, payload and length are left unchanged,
 * and errno is set.
 */
int proto_recv_packet(int fd, CHLA_PACKET_HEADER *hdr, void **payload){
	int bytes_writen = 0;
	int amount;
	while(bytes_writen < sizeof(CHLA_PACKET_HEADER)){
		amount = rio_readn(fd, (hdr + bytes_writen), sizeof(CHLA_PACKET_HEADER));
		if(amount <= 0){
			return -1;
		}
		bytes_writen+=amount;
	}
	uint32_t length = ntohl(hdr -> payload_length);

	//not reading payload properly
	// char temp[length];
	bytes_writen = 0;
	if(length > 0){
		*payload = malloc(length);

		while(bytes_writen < length){
			amount = rio_readn(fd, *payload, length);
			if(amount <= 0){
				free(payload);
				return -1;
			}
			bytes_writen += amount;
		}
	}
	debug("%s\n", (char *)*payload);
	if(length == 0)
		debug("type= %s, payload_length= %d, msgid= %d\n", typetostring(hdr), length, ntohl(hdr -> msgid));
	else
		debug("type= %s, payload_length= %d, msgid= %d, payload:[%s]\n", typetostring(hdr), length, ntohl(hdr -> msgid), (char *)payload);

	debug("recieved\n");
	return 0;
}




char *typetostring(CHLA_PACKET_HEADER *hdr){
	char *typestr;
	switch(hdr -> type){
		//client to server
		case CHLA_LOGIN_PKT:
			typestr = "login";
			break;
		case CHLA_LOGOUT_PKT:
			typestr = "logout";
			break;
		case CHLA_USERS_PKT:
			typestr = "users";
			break;
		case CHLA_SEND_PKT:
			typestr = "send";
			break;
			//server to client
		case CHLA_ACK_PKT:
			typestr = "ack";
			break;
		case CHLA_NACK_PKT:
			typestr = "nack";
			break;
		case CHLA_MESG_PKT:
			typestr = "mesg";
			break;
		case CHLA_RCVD_PKT:
			typestr = "rcvd";
			break;
		case CHLA_BOUNCE_PKT:
			typestr = "bounce";
			break;
		default:
			typestr = NULL;
			break;
	}
	return typestr;
}