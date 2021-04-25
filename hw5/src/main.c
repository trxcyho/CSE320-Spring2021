#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
//extra
#include <getopt.h>
#include <ctype.h>
#include <csapp.h>

#include "debug.h"
#include "server.h"
#include "globals.h"

static void terminate(int);

//prototypes
void terminate_handler(int sig);
/*
 * "Charla" chat server.
 *
 * Usage: charla <port>
 */
int main(int argc, char* argv[]){

    //install sigaction with SIGUP
    struct sigaction act, old_act;
    act.sa_handler = terminate_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART;
    if(sigaction(SIGHUP, &act, &old_act)){
        perror("sigaction");
        return 1;
    }

    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.
    char *port_num = NULL;
    int p_flag = 0;
    int c;
    while((c = getopt(argc, argv, "p:")) > 0){
        switch(c){
            case 'p':
                //read next few digits
                p_flag = 1;
                port_num = optarg;
                for(int i = 0; i < strlen(port_num); i++){
                    if(!isdigit(port_num[i])){
                        port_num = NULL;
                        break;
                    }
                }
                break;
            //-h specify host if you want
            default:
                printf("invalid option  %c\n", c);
                break;
        }
    }
    if(p_flag == 0){
        printf("USAGE: -p <port> [-h <host>]");
        return 1;
    }


    // Perform required initializations of the client_registry and
    // player_registry.
    user_registry = ureg_init();
    client_registry = creg_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function charla_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.

    //if port number is null of less than 1024, printf error and return?

    int server = Open_listenfd(port_num); //conect to designated port_num
    if(server < 0){//port_num not valid
        debug("port number not valid");
        return 1;
    }

    int client_socket;
    struct sockaddr_in client;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int *socket;

    while((client_socket = accept(server, (struct sockaddr *)&client, &addrlen)) > 0){
        pthread_t thread;
        socket = malloc(sizeof(int));
        *socket = client_socket;

        if(pthread_create(&thread, NULL, chla_client_service, socket) < 0){//error
            printf("failed to create thread\n");
            return 1;
        }

    }
    //throw error if client socket not valid?
    //terminate and exitsuccess???


    fprintf(stderr, "You have to finish implementing main() "
	    "before the server will function.\n");

    terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
static void terminate(int status) {
    // Shut down all existing client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);

    // Finalize modules.
    creg_fini(client_registry);
    ureg_fini(user_registry);

    debug("%ld: Server terminating", pthread_self());
    exit(status);
}

void terminate_handler(int sig){
    terminate(EXIT_SUCCESS);//what status to terminate?

}