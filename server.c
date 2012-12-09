/*
 * A multithreaded HTTP webserver
 * Project 5
 * Nayef Copty & K Al Najar
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>

#include <pthread.h>

#include "server.h"
#include "thread-pool/threadpool.h"

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
    signal(SIGPIPE, SIG_IGN);
    
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

	//printf("The ID of main thread is: %u\n", (unsigned int)pthread_self());

	if ((connfd = accept(socketfd, (struct sockaddr *)&clientaddr, (socklen_t *)&clientlen)) <= 0) {
	    fprintf(stderr, "Error acceping connection.\n");
	    exit(1);
	}

	// execute request in a different thread
	struct future * f;
	f = thread_pool_submit(pool, (thread_pool_callable_func_t)process_http, &connfd);
    }

    thread_pool_shutdown(pool);

    return 0;
}

static void process_http(int *fd)
{
    int is_static;
    //struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;
 
    /* Read request line and headers */
    Rio_readinitb(&rio, *fd);
    Rio_readlineb(&rio, buf, MAXLINE);

    sscanf(buf, "%s %s %s", method, uri, version);

    if (strcasecmp(method, "GET")) {
       clienterror(*fd, method, "501", "Not Implemented", "Tiny does not implement this method");
        return;
    }
    read_requesthdrs(&rio);

    /* Parse URI from GET request */
    is_static = parse_uri(uri, filename, cgiargs);

    /* /proc/loadavg */
    if (strstr(uri, "loadavg") != NULL) {

	FILE *fp;
	fp = fopen("/proc/loadavg", "r");

	if (fp) {

	    char loadbuf[256];
	    fgets(loadbuf, sizeof(loadbuf), fp);

	    float loadavg1;
	    float loadavg2;
	    float loadavg3;
	    int running;
	    char sep;
	    int total;

	    sscanf(loadbuf, "%f %f %f %d %c %d", &loadavg1, &loadavg2, &loadavg3, &running, &sep, &total);

	    char load_json[256];
	    sprintf(load_json, "{\"total_threads\": \"%d\", \"loadavg\": [\"%.2f\", \"%.2f\", \"%.2f\"], \"running_threads\": \"%d\"}", total, loadavg1, loadavg2, loadavg3, running);

	    char callbackbuf[256];	   
	    if (parse_callback(uri, callbackbuf) > 0) {
		char returnbuf[256];
		sprintf(returnbuf, "%s({%s})", callbackbuf, load_json);
		response(*fd, returnbuf, "application/javascript");
	    }
	    
	    else
		response(*fd, load_json, "application/json");
		
	}
	
	fclose(fp);	
    }

    /* /proc/meminfo */
    else if (strstr(uri, "meminfo") != NULL) {
	
	FILE *fp;
	fp = fopen("/proc/meminfo", "r");

	if (fp) {
	    
	    char line[256];
	    char membuf[1024];
	    strcpy(membuf, "{");
	    int flag = 0;
	    
	    while (fgets(line, sizeof(line), fp)) {

		if (flag != 0) {
		    strcat(membuf, ", ");
		}

		char title[64];
		long memory;
		char skip[5];
		
		sscanf(line, "%s %lu %s", title, &memory, skip);

		title[strlen(title)-1] = '\0';
		
		char tempbuf[256];
		sprintf(tempbuf, "\"%s\": \"%lu\"", title, memory);
		strcat(membuf, tempbuf);
		flag = 1;
	    }

	    strcat(membuf, "}");
	    
	    char callbackbuf[256];	   
	    if (parse_callback(uri, callbackbuf) > 0) {
		char returnbuf[256];
		sprintf(returnbuf, "%s({%s})", callbackbuf, membuf);
		response(*fd, returnbuf, "application/javascript");
	    }
	    
	    else
		response(*fd, membuf, "application/json");
	}
	
	fclose(fp);
    }

    else {

	///// THROW OUT AN ERROR
	char b[1024];
	sprintf(b, "it is: %d with uri: %s", strncmp(uri, "/loadavg", strlen("/loadavg")), uri);
	response(*fd, b, "application/json");
    }

    
    /*if (stat(filename, &sbuf) < 0) {                     //line:netp:doit:beginnotfound
	clienterror(fd, filename, "404", "Not found",
		    "Tiny couldn't find this file");
	return;
    }                                                    //line:netp:doit:endnotfound

    if (is_static) { // Serve static content
	if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { //line:netp:doit:readable
	    clienterror(fd, filename, "403", "Forbidden",
			"Tiny couldn't read the file");
	    return;
	}
	serve_static(fd, filename, sbuf.st_size);        //line:netp:doit:servestatic
    }
    else { // Serve dynamic content
	if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { //line:netp:doit:executable
	    clienterror(fd, filename, "403", "Forbidden",
			"Tiny couldn't run the CGI program");
	    return;
	}
	serve_dynamic(fd, filename, cgiargs);            //line:netp:doit:servedynamic
    }*/

    close(*fd);
}

/*
 * parse_callback - parse URI and returns callback buffer size
 *                  return 0 if no callback exists
 */
static size_t parse_callback(char *uri, char *callback)
{
    char *ptr;
    ptr = index(uri, '?');

    if (ptr && strncmp(ptr, "?callback=", sizeof("?callback=")) > 0) {

	ptr = index(uri, '=');

	if (ptr) {
	    ptr++;

	    int i = 0;

	    while (*ptr && *ptr != '&') {

		int c = ptr[0];
		if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122) || c == 46 || (c >= 48 && c <= 57) || c == 95) {
		    callback[i] = *ptr;
		    i++;
		    ptr++;
		}

		else
		    return 0;
	    }
	    
	    return sizeof(callback);
	}

	else
	    return 0;
	
    }

    else
	return 0;
}


/*
 * read_requesthdrs - read and parse HTTP request headers
 */
void read_requesthdrs(rio_t *rp) 
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {
	Rio_readlineb(rp, buf, MAXLINE);
	printf("%s", buf);
    }
    return;
}

/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
int parse_uri(char *uri, char *filename, char *cgiargs) 
{
    char *ptr;

    if (!strstr(uri, "cgi-bin")) {  /* Static content */ //line:netp:parseuri:isstatic
	strcpy(cgiargs, "");                             //line:netp:parseuri:clearcgi
	strcpy(filename, ".");                           //line:netp:parseuri:beginconvert1
	strcat(filename, uri);                           //line:netp:parseuri:endconvert1
	if (uri[strlen(uri)-1] == '/')                   //line:netp:parseuri:slashcheck
	    strcat(filename, "home.html");               //line:netp:parseuri:appenddefault
	return 1;
    }

    else {  /* Dynamic content */                        //line:netp:parseuri:isdynamic
	ptr = index(uri, '?');                           //line:netp:parseuri:beginextract
	if (ptr) {
	    strcpy(cgiargs, ptr+1);
	    *ptr = '\0';
	}
	else 
	    strcpy(cgiargs, "");                         //line:netp:parseuri:endextract
	strcpy(filename, ".");                           //line:netp:parseuri:beginconvert2
	strcat(filename, uri);                           //line:netp:parseuri:endconvert2
	return 0;
    }
}

/*
 * Build and send an HTTP response
 */
void response(int fd, char *msg, char *content_type)
{
    char buf[MAXLINE];
    char body[MAXBUF];

    sprintf(body, "%s", msg);
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: %s\r\n", content_type);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));   
    Rio_writen(fd, body, strlen(body));
}

/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
/* $end clienterror */

