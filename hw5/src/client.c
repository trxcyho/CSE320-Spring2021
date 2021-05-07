#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "debug.h"
#include "server.h"
#include "globals.h"
#include "user_registry.h"
#include "user.h"
#include "protocol.h"
#include <csapp.h>

char *pkttypestr(CHLA_PACKET_HEADER *hdr);

struct client {
	int fd;
	int ref_count;
	int loggedin; //1=logggedin 0 = not logged in
	USER* user;
	MAILBOX *mailbox;
	pthread_mutex_t client_mutex;
};


CLIENT *client_create(CLIENT_REGISTRY *creg, int fd){
	//lock client when creating?
	CLIENT *new_client = malloc(sizeof(CLIENT));
	if(new_client == NULL)
		return NULL;
	new_client -> fd = fd;
	new_client -> ref_count = 1;
	new_client -> loggedin = 0;
	new_client -> user = NULL;
	new_client -> mailbox = NULL;
	pthread_mutex_init(&new_client -> client_mutex, NULL);
	debug("Starting client service for fd: %d", fd);
	return new_client;
}

/*
 * Increase the reference count on a CLIENT by one.
 *
 * @param client  The CLIENT whose reference count is to be increased.
 * @param why  A string describing the reason why the reference count is
 * being increased.  This is used for debugging printout, to help trace
 * the reference counting.
 * @return  The same CLIENT that was passed as a parameter.
 */
CLIENT *client_ref(CLIENT *client, char *why){
	pthread_mutex_lock(&client -> client_mutex);
	client-> ref_count = (client -> ref_count) +1;
	pthread_mutex_unlock(&client -> client_mutex);
	debug("Increased reference count on client (%d -> %d): %s\n", (client -> ref_count) -1, client -> ref_count, why);
	return client;
}

/*
 * Decrease the reference count on a CLIENT by one.  If after
 * decrementing, the reference count has reached zero, then the CLIENT
 * and its contents are freed.
 *
 * @param client  The CLIENT whose reference count is to be decreased.
 * @param why  A string describing the reason why the reference count is
 * being decreased.  This is used for debugging printout, to help trace
 * the reference counting.
 */
void client_unref(CLIENT *client, char *why){
	pthread_mutex_lock(&client -> client_mutex);
	client-> ref_count = (client -> ref_count) -1;
	pthread_mutex_unlock(&client -> client_mutex);
	debug("Decreased reference count on client (%d -> %d): %s\n", (client -> ref_count) +1, client -> ref_count, why);
	if(client -> ref_count == 0){
		debug("Free client at fd:[%d] because ref count = 0", client -> fd);
		free(client);
	}
}

/*
 * Log this CLIENT in under a specified handle.
 * The handle is registered with the user registry, creating a new
 * USER object corresponding to the handle if one did not already
 * exist.  A MAILBOX is also created and retained by the CLIENT.
 * The login fails if the CLIENT is already logged in or there is already
 * some other CLIENT that is logged under the specified handle.
 * Otherwise, the login is successful and the CLIENT is marked as "logged in".
 *
 * @param CLIENT  The CLIENT that is to be logged in.
 * @param handle  The handle under which the CLIENT is to be logged in.
 * @return 0 if the login operation is successful, otherwise -1.
 */
int client_login(CLIENT *client, char *handle){
	debug("client_login");
	//check if client logged out
	if(client -> loggedin == 1)
		return -1;

	//lock current client?
	// pthread_mutex_lock(&client -> client_mutex);
	pthread_mutex_t mutex;
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_lock(&mutex);
	USER * user = ureg_register(user_registry, handle);
	CLIENT **client_arr = creg_all_clients(client_registry);//from global
	if(client_arr == NULL)
		return -1;
	//loop through client_arr and see if any client logged in -> user handle equaivalent
	int i = 0;
	int exists = 0;
	while(client_arr[i] != NULL){
		debug("client while login");
		client_unref(client_arr[i], "for reference in clients list being discarded");
		if(client_arr[i] -> loggedin == 1){
			if(strcmp(user_get_handle(client_arr[i]->user), handle) == 0){
				debug("Can't login, user %s exists", handle);
				exists = 1;
			}
		}
		i++;
	}
	if (exists == 1)
		return -1;

	free(client_arr);
	debug("Login user %s", handle);
	//register handle with user reg

	//create MAILBOX
	client -> loggedin = 1;
	client -> user = user;
	client -> mailbox = mb_init(handle);
	pthread_mutex_unlock(&mutex);
	// pthread_mutex_unlock(&client -> client_mutex);
	debug("Client logged in (%s) mailbox:%p", handle, client -> mailbox);
	return 0;
}

/*
 * Log out this CLIENT.  If the client was not logged in, then it is
 * an error.  The references to the USER and MAILBOX that were saved
 * when the CLIENT was logged in are discarded.
 *
 * @param client  The CLIENT that is to be logged out.
 * @return 0 if the client was logged in and has been successfully
 * logged out, otherwise -1.
 */
int client_logout(CLIENT *client){
	debug("in clientlogout");
	//check if client loggin
	if(client -> loggedin == 0)
		return -1;
	//user dereference, mailbox deref
	pthread_mutex_lock(&client -> client_mutex);
	user_unref(client ->user, "client logged out");
	mb_unref(client-> mailbox, "client logged out");
	mb_shutdown(client -> mailbox);
	client->user=  NULL;
	client -> mailbox = NULL;
	client -> loggedin = 0;
	pthread_mutex_unlock(&client -> client_mutex);
	return 0;
}


USER *client_get_user(CLIENT *client, int no_ref){
	debug("in client get user");
	if(client == NULL)
		return NULL;
	if(client->loggedin == 0)
		return NULL;
	if(no_ref == 0)
		user_ref(client -> user, "client get user (no_ref = 0)");
		//increment ref count on user
	return client -> user;
}

/*
 * Get the MAILBOX for the specified logged-in CLIENT.
 *
 * @param client  The CLIENT from which to get the MAILBOX.
 * @param no_ref  If nonzero, then the reference count on the returned
 * MAILBOX is *not* incremented.  This is a convenience feature that
 * should only be used if the caller knows that the CLIENT cannot
 * be logged out until the MAILBOX object is no longer needed.
 * Otherwise, the caller should pass a zero value for no_ref and
 * accept the responsibility for decrementing the reference count
 * when finished with the MAILBOX object.
 * @return  The MAILBOX that the CLIENT is currently using,
 * otherwise NULL if the client is not currently logged in.
*/
MAILBOX *client_get_mailbox(CLIENT *client, int no_ref){
	debug("in client get mailbox");
	if(client->loggedin == 0)
		return NULL;
	pthread_mutex_lock(&client -> client_mutex);
	if(no_ref == 0){
		pthread_mutex_unlock(&client -> client_mutex);
		mb_ref(client-> mailbox, "client get mailbox (no_ref = 0)");
	}
	pthread_mutex_unlock(&client -> client_mutex);
	return client -> mailbox;
}

/*
 * Get the file descriptor for the network connection associated with
 * this CLIENT.  The file descriptor is set at the time the CLIENT
 * is created and does not change.
 *
 * @param client  The CLIENT for which the file descriptor is to be
 * obtained.
 * @return the file descriptor.
 */
int client_get_fd(CLIENT *client){
	return client -> fd;
}

/*
 * Send a packet to a client.  Exclusive access to the network connection
 * is obtained for the duration of this operation, to prevent concurrent
 * invocations from corrupting each other's transmissions.  To prevent
 * such interference, only this function should be used to send packets to
 * the client, rather than the lower-level proto_send_packet() function.
 *
 * @param client  The CLIENT who should be sent the packet.
 * @param pkt  The header of the packet to be sent.
 * @param data  Data payload to be sent, or NULL if none.
 * @return 0 if transmission succeeds, -1 otherwise.
 */
int client_send_packet(CLIENT *user, CHLA_PACKET_HEADER *pkt, void *data){
	debug("in client send packet");
	pthread_mutex_lock(&user -> client_mutex);
	uint32_t length = htonl(pkt -> payload_length);
	//write header
	debug("Send packet (clientfd=%d, type=%s)", user->fd, pkttypestr(pkt));
	if(rio_writen(user-> fd, pkt, sizeof(CHLA_PACKET_HEADER)) <=0 ){
		debug("error send");
		return -1; //rio_writen sets errno
	}
	//write payload
	if(length > 0){
		if(rio_writen(user->fd, data, length) <= 0){
			debug("error send2");
			return -1;
		}

	}
	pthread_mutex_unlock(&user-> client_mutex);
	if(length == 0)
		debug("type= %s, payload_length= %d, msgid= %d\n", pkttypestr(pkt), length, htonl(pkt -> msgid));
	else
		debug("type= %s, payload_length= %d, msgid= %d, payload:[%s]\n", pkttypestr(pkt), length, htonl(pkt -> msgid), (char *)data);

	return 0;
}

/*
 * Send an ACK packet to a client.  This is a convenience function that
 * streamlines a common case.
 *
 * @param client  The CLIENT who should be sent the packet.
 * @param msgid  Message ID (in host byte order) to which the ACK pertains.
 * @param data  Pointer to the optional data payload for this packet,
 * or NULL if there is to be no payload.
 * @param datalen  Length of the data payload, or 0 if there is none.
 * @return 0 if transmission succeeds, -1 otherwise.
 */
int client_send_ack(CLIENT *client, uint32_t msgid, void *data, size_t datalen){
	debug("in client_send_ack");
	CHLA_PACKET_HEADER *packet = malloc(sizeof(CHLA_PACKET_HEADER));
	if(packet == NULL)
		return -1;
	struct timespec ctime;
	clock_gettime(CLOCK_REALTIME, &ctime);

	packet -> type = CHLA_ACK_PKT;
	packet -> payload_length = htonl(datalen);
	packet -> msgid = htonl(msgid);
	packet -> timestamp_sec = ctime.tv_sec;
	packet -> timestamp_nsec = ctime.tv_nsec;
	if(client_send_packet(client, packet, data) == -1){
		free(packet);
		return -1;
	}
	free(packet);
	return 0;
}

/*
 * Send an NACK packet to a client.  This is a convenience function that
 * streamlines a common case.
 *
 * @param client  The CLIENT to be sent the packet.
 * @param msgid  Message ID to which the ACK pertains.
 * @return 0 if transmission succeeds, -1 otherwise.
 */
int client_send_nack(CLIENT *client, uint32_t msgid){
	debug("in client_send nack");
	CHLA_PACKET_HEADER *packet = malloc(sizeof(CHLA_PACKET_HEADER));
	if(packet == NULL)
		return -1;
	struct timespec ctime;
	clock_gettime(CLOCK_REALTIME, &ctime);
	packet -> type = CHLA_NACK_PKT;
	packet -> payload_length = 0;
	packet -> msgid = htonl(msgid);
	packet -> timestamp_sec = htonl(ctime.tv_sec);
	packet -> timestamp_nsec = htonl(ctime.tv_nsec);
	if(client_send_packet(client, packet, NULL) == -1){
		free(packet);
		return -1;
	}
	free(packet);
	return 0;
}

char *pkttypestr(CHLA_PACKET_HEADER *hdr){
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
			typestr = "invalid"; //should never be printed
			break;
	}
	return typestr;
}
