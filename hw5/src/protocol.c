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
	uint32_t length = hdr -> payload_length;

	if(length == 0)
		debug("type= %s, payload_length= %d, msgid= %d\n", typetostring(hdr), length, hdr -> msgid);
	else
		debug("type= %s, payload_length= %d, msgid= %d, payload:[%s]\n", typetostring(hdr), length, hdr -> msgid, (char *)payload);

	//write header
	if(rio_writen(fd, hdr, sizeof(CHLA_PACKET_HEADER)) <= 0)
		return -1; //rio_writen sets errno

	//write payload
	if(length > 0){
		if(rio_writen(fd, payload, length) <= 0)
			return -1;
	}
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
	if(rio_readn(fd, hdr, sizeof(CHLA_PACKET_HEADER)) <= 0){
		return -1;
	}
	uint32_t length = ntohl(hdr -> payload_length);

	//not reading payload properly
	if(length > 0){
		*payload = malloc(length);
		if(rio_readn(fd, *payload, length) <= 0){
			free(*payload);
			return -1;
		}
	}

	if(length == 0)
		debug("type= %s, payload_length= %d, msgid= %d\n", typetostring(hdr), length, ntohl(hdr -> msgid));
	else
		debug("type= %s, payload_length= %d, msgid= %d, payload:[%s]\n", typetostring(hdr), length, ntohl(hdr -> msgid), (char *)payload);


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