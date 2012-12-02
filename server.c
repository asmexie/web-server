/*
 * A multithreaded HTTP webserver
 * Project 5
 * Nayef Copty & K Al Najar
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "rio.h"
#include "thread-pool/threadpool.h"

#define THREADS 20

static void usage(char *progname)
{
    printf("Usage: %s -h\n"
	   " -h print this help\n"
	   " -p port to accept HTTP clients requests\n"
	   " -r relayhost:port connect to relayhost on specified port\n"
	   " -R path specifies root directory of server reachable under '/files' prefix\n",
	   progname);
    
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    if (argc < 2)
	usage(argv[0]);
    
    long port;

    char c;
    while ((c = getopt(argc, argv, "p:r:R:")) != -1) {
	
	switch (c) {
	    
	case 'h':
	    usage(argv[0]);
	    break;

	case 'p': {
	    char *garbage = NULL;
	    port = strtol(optarg, &garbage, 10);

	    switch (errno) {
	    case ERANGE:
		printf("Please enter a valid port number");
		return -1;
	    case EINVAL:
		printf("Please enter a valid port number");
		return -1;
	    }

	    break;
	}
	    
	case 'r': 
	    // relay host in optarg
	    break;
	case 'R':
	    // value (path) specifies the root dir of the server in optarg
	    break;
	case '?':
	default:
	    usage(argv[0]);
	}
    }

    // socket setup -- move to a different function perhaps?

    int socketfd, connfd, optval = 1;
    struct sockaddr_in serveraddr;
    struct sockaddr_in clientaddr;

    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	fprintf(stderr, "Error opening socket.\n");
	exit(1);
    }

    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int)) < 0) {
	fprintf(stderr, "Error in socket. %s.\n", strerror(errno));
	exit(1);
    }
    
    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short) port);
    
    if (bind(socketfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
	fprintf(stderr, "Error binding socket. %s.\n", strerror(errno));
	exit(1);
    }

    if (listen(socketfd, 1024) < 0) {
	fprintf(stderr, "Error listening to socket.\n");
	exit(1);
    }

    
    // thread pool setup -- perhaps new function?
    struct thread_pool *pool = thread_pool_new(THREADS);
    
    for (;;) {
	// new connected socket
	int clientlen = sizeof(clientaddr);
	if ((connfd = accept(socketfd, (struct sockaddr *)&clientaddr, (socklen_t *)&clientlen) < 0)) {
	    fprintf(stderr, "Error acceping connection.\n");
	    exit(1);
	}
	close(connfd);	
	// run connfd
	// close connection if non-persistent
    }

    thread_pool_shutdown(pool);

    return 0;
}
