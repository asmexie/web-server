#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>

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

static void open_socket(int port)
{
    int listenfd, optval = 1;
    struct sockaddr_in serveraddr;

    // open socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0) < 0)) {
	fprintf(stderr, "Error opening socket.\n");
	exit(EXIT_FAILURE);
    }
    
    if (setsockopt(listenfd, SOL,SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) < 0) {
	fprintf(stderr, "Error in socket.\n");
	exit(EXIT_FAILURE);
    }

    bzero((char *)&aserveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short) port);
    if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
	fprintf(stderr, "Error binding socket address to port.\n");
	exit(EXIT_FAILURE);
    }

    if (listen(listenfd, LISTENQ) < 0) {
	fprintf(stderr, "Error listening to socket.\n");
	exit(EXIT_FAILURE);
    }

    return listenfd;    
}
	   

int main(int argc, char **argv)
{
    char c;
    while ((c = getopt(argc, argv, "p:r:R:")) != -1) {
	
	switch (c) {

	case 'h':
	    usage(argv[0]);
	    break;

	case 'p': {

	    char *garbage = NULL;
	    long port = strtol(optarg, &garbage, 10);

	    switch (errno) {
	    case ERANGE:
		printf("Please enter a valid port number");
		return EXIT_FAILURE;
	    case EINVAL:
		printf("Please enter a valid port number");
		return EXIT_FAILURE;
	    }

	    printf("%lu\n", port);
	    // handle port here

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

    int listenfd, connfd, clientlen;
    listenfd = open_listfd(port);

    return 0;
}
