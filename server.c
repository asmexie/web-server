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
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/wait.h>

#include "server.h"
#include "thread-pool/threadpool.h"

static char *custom_path;

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
	    // path that specifies the root dir of the server
	    custom_path = strdup(optarg);
	    break;

	case '?':
	default:
	    usage(argv[0]);
	}
    }

    if (custom_path == NULL)
	custom_path = "files";

    list_init(&memory_list);

    int socketfd, optval = 1;
    intptr_t connfd;



    /* IPv6 */
    /*
    struct addrinfo *ai;
    struct addrinfo hints;
    memset (&hints, '\0', sizeof (hints));
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    hints.ai_socktype = SOCK_STREAM;
    int e = getaddrinfo (NULL, "echo", &hints, &ai);
    if (e != 0)
	error (EXIT_FAILURE, 0, "getaddrinfo: %s", gai_strerror (e));
    int nfds = 0;
    struct addrinfo *runp = ai;
    while (runp != NULL) {
	++nfds;
	runp = runp->ai_next;
    }

    struct pollfd fds[nfds];
    
    for (nfds = 0, runp = ai; runp != NULL; runp = runp->ai_next) {	

	fds[nfds].fd = socket (runp->ai_family, runp->ai_socktype, runp->ai_protocol);
	if (fds[nfds].fd == -1)
	    error(EXIT_FAILURE, errno, "socket");

	fds[nfds].events = POLLIN;
	int opt = 1;
	setsockopt (fds[nfds].fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));

	if (bind (fds[nfds].fd,runp->ai_addr, runp->ai_addrlen) != 0)
	    {
		if (errno != EADDRINUSE)
		    error (EXIT_FAILURE, errno, "bind");
		close (fds[nfds].fd);
	    }

	else {
	    if (listen (fds[nfds].fd, SOMAXCONN) != 0)
		error (EXIT_FAILURE, errno, "listen");
	    ++nfds;
	}
    }
    freeaddrinfo (ai);
    */
	    
		  




    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	fprintf(stderr, "Error opening socket.\n");
	exit(1);
    }

    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int)) < 0) {
	fprintf(stderr, "Error in socket. %s.\n", strerror(errno));
	exit(1);
    }

    struct sockaddr_in serveraddr;
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

    struct thread_pool *pool = thread_pool_new(THREADS);
    struct sockaddr_in clientaddr;
    
    for (;;) {
	// new connected socket
	int clientlen = sizeof(clientaddr);
	
	if ((connfd = accept(socketfd, (struct sockaddr *)&clientaddr, (socklen_t *)&clientlen)) <= 0) {
	    fprintf(stderr, "Error acceping connection.\n");
	    exit(1);
	}
	
	// execute request in a different thread
	thread_pool_submit(pool, (thread_pool_callable_func_t)process_http, (int *)connfd);
	
	/*while (future_get(f)) {
	  future_free(f);
	  }*/
    }
    
    //close(connfd);
    thread_pool_shutdown(pool);
    
    return 0;
}

static void process_http(int fd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;
    
    char pathbuf[64];
    sprintf(pathbuf, "./%s/", custom_path); 
    
    Rio_readinitb(&rio, fd);
    
    while (1) {
	
	/* Read request line and headers */
	if (Rio_readlineb(&rio, buf, MAXLINE) <= 0)
	    break;
	
	sscanf(buf, "%s %s %s", method, uri, version);
	
	if (strcasecmp(method, "GET")) {
	    clienterror(fd, method, "501", "Not Implemented", "Server does not implement this method", version);
	    continue;
	}
	
	ssize_t size = read_requesthdrs(&rio);	
	if (size < 0) {
	    if (strncmp(version, "HTTP/1.0", 8) == 0) {
		break;
	    }
	    
	    else {
		continue;
	    }
	}

	/* Parse URI from GET request */
	is_static = parse_uri(uri, filename, cgiargs);
	
	/* /proc/loadavg */
	if (strncmp(uri, "/loadavg\0", sizeof("/loadavg\0")-1) == 0 || strncmp(uri, "/loadavg?", sizeof("/loadavg?")-1) == 0) {

	    FILE *fp;
	    fp = fopen("/proc/loadavg", "r");
	
	    if (fp) {
		
		char loadbuf[256];
		fgets(loadbuf, sizeof(loadbuf), fp);
		fclose(fp);
		
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
		    sprintf(returnbuf, "%s(%s)", callbackbuf, load_json);
		    response_ok(fd, returnbuf, "application/javascript", version);
		}
			
		else
		    response_ok(fd, load_json, "application/json", version);
	    }
	}
	
	/* /proc/meminfo */
	else if (strncmp(uri, "/meminfo\0", sizeof("/meminfo\0")-1) == 0 || strncmp(uri, "/meminfo?", sizeof("/meminfo?")-1) == 0) {
		
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

		fclose(fp);
		strcat(membuf, "}");
			
		char callbackbuf[256];	   
		if (parse_callback(uri, callbackbuf) > 0) {
		    char returnbuf[256];
		    sprintf(returnbuf, "%s(%s)", callbackbuf, membuf);
		    response_ok(fd, returnbuf, "application/javascript", version);
		}
			
		else
		    response_ok(fd, membuf, "application/json", version);
	    }
	}

	else if (strncmp(filename, pathbuf, strlen(pathbuf)) == 0) {

	    // check that filename does not contain '..'
	    if (strstr(filename, "..") != NULL)
		clienterror(fd, filename, "403", "Forbidden", "Server couldn't read the file", version);

	    if (stat(filename, &sbuf) < 0)
		clienterror(fd, filename, "404", "Not found", "Page Not Found", version);

	    if (is_static) {
		
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
		    clienterror(fd, filename, "403", "Forbidden", "Server couldn't read the file", version);
		
		serve_static(fd, filename, sbuf.st_size);
	    }
    
	    else {
		
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
		    clienterror(fd, filename, "403", "Forbidden", "Server couldn't run the CGI program", version);
		
		serve_dynamic(fd, filename, cgiargs);
	    }
	}

	else if (strncmp(uri, "/runloop", strlen("/runloop")) == 0) {

	    pid_t pid;
	    pid = fork();

	    if (pid == 0) {
		while (1) {
		    sleep(15);
		    break;
		}
	    }

	    else if (pid < 0) {
		fprintf(stderr, "Fork Error.\n");
		break;
	    }

	    int status;
	    wait(&status);

	    // child exited
	    if (WIFEXITED(status)) {
		FILE *fp;
		fp = fopen("/proc/loadavg", "r");
		
		if (fp) {		    
		    char loadbuf[256];
		    fgets(loadbuf, sizeof(loadbuf), fp);
		    fclose(fp);
		    
		    float loadavg1;
		    float loadavg2;
		    float loadavg3;
		    
		    sscanf(loadbuf, "%f %f %f", &loadavg1, &loadavg2, &loadavg3);
		    response_ok(fd, "<html>\n<body>\n<p>There was a load average spike.</p>\n</body>\n</html>", "text/html", version);
		}
	    }

	    else {
		char respbuf[256];
		sprintf(respbuf, "<html>\n<body>\n<p>There was a problem in your request.</p>\n</body>\n</html>");
		response_ok(fd, respbuf, "text/html", version);
	    }
	}

	else if (strncmp(uri, "/allocanon", strlen("/allocanon")) == 0) {

	    void *block = mmap(NULL, 67108864, PROT_READ, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	    if (!block)
		perror("mmap() failed\n");

	    else {

		struct memory *e = (struct memory *)malloc(sizeof(struct memory));
		e->block = block;
		list_push_back(&memory_list, &e->elem);

		char buf[256];
		sprintf(buf, "<html>\n<body>\n<p>64MB allocated in the heap.</p>\n</body>\n</html>");
		response_ok(fd, buf, "text/html", version);
	    }
	}

	else if (strncmp(uri, "/freeanon", strlen("/freeanon")) == 0) {

	    if (list_size(&memory_list) > 0) {

		struct list_elem *e = list_pop_back(&memory_list);
		struct memory *mem = list_entry(e, struct memory, elem);

		int success = munmap(mem->block, sizeof(*mem->block));
		
		if (success < 0)
		    fprintf(stderr, "munmap() failed %s\n", strerror(errno));

		else {
		    char buf[256];
		    sprintf(buf, "<html>\n<body>\n<p>64MB freed successfully.</p>\n</body>\n</html>");
		    response_ok(fd, buf, "text/html", version);
		}
	    }

	    else {
		char buf[256];
		sprintf(buf, "<html>\n<body>\n<p>You must allocate memory first using /allocanon</p>\n</body>\n</html>");
		response_ok(fd, buf, "text/html", version);
	    }
	}

	else
	    clienterror(fd, filename, "404", "Not Found", "Not found", version);
	
	if (strncmp(version, "HTTP/1.0", 8) == 0) {
	    break;
	}
    }
    
    close(fd);
}

/*
 * read_requesthdrs - read and parse HTTP request headers
 */
ssize_t read_requesthdrs(rio_t *rp) 
{
    char buf[MAXLINE];

    ssize_t size = Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {
	size = Rio_readlineb(rp, buf, MAXLINE);
	//printf("Buf: %s", buf);
    }
    return size;
}

/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
int parse_uri(char *uri, char *filename, char *cgiargs) 
{
    char *ptr;

    if (!strstr(uri, "cgi-bin")) {
	strcpy(cgiargs, "");
	strcpy(filename, ".");
	strcat(filename, uri);
	if (uri[strlen(uri)-1] == '/') {
	    char pathbuf[64];
	    sprintf(pathbuf, "%s/home.html", custom_path);
	    strcat(filename, pathbuf);
	}
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
void response_ok(int fd, char *msg, char *content_type, char *version)
{
    char buf[MAXLINE];
    char body[MAXBUF];

    sprintf(body, "%s", msg);
    sprintf(buf, "%s 200 OK\r\n", version);
    if (strncmp(version, "HTTP/1.0", strlen("HTTP/1.0")) == 0)
	sprintf(buf, "Connection: close\r\n");
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
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg, char *version) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>Sysstatd Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "%s %s %s\r\n", version, errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
/* $end clienterror */

/*
 * parse_callback - parse URI and returns callback buffer size
 *                  return 0 if no callback exists
 */
static size_t parse_callback(char *uri, char *callback)
{
    char *ptr;
    ptr = index(uri, '?');
    fflush(stdout);
    
    if (ptr) {

	while (ptr) {
	    ptr++;
	    if (strncmp(ptr, "callback=", sizeof("callback=")-1) == 0) {
		
		ptr = index(ptr, '=');
		int pass = 0;
		
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
			    pass = 1;
		    }

		    if (pass == 0) {
			callback[i] = '\0';
			return sizeof(callback);
		    }
		}
		
	    }

	    else
		ptr = index(ptr, '&');
	}
    }
    
    return 0;
}

/*
 * serve_static - copy a file back to the client 
 */
/* $begin serve_static */
void serve_static(int fd, char *filename, int filesize) 
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];
 
    /* Send response headers to client */
    get_filetype(filename, filetype);       //line:netp:servestatic:getfiletype
    sprintf(buf, "HTTP/1.0 200 OK\r\n");    //line:netp:servestatic:beginserve
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));       //line:netp:servestatic:endserve

    /* Send response body to client */
    srcfd = open(filename, O_RDONLY, 0);    //line:netp:servestatic:open
    srcp = mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);//line:netp:servestatic:mmap
    close(srcfd);                           //line:netp:servestatic:close
    Rio_writen(fd, srcp, filesize);         //line:netp:servestatic:write
    munmap(srcp, filesize);                 //line:netp:servestatic:munmap
}

/*
 * get_filetype - derive file type from file name
 */
void get_filetype(char *filename, char *filetype) 
{
    if (strstr(filename, ".html"))
	strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
	strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg"))
	strcpy(filetype, "image/jpeg");
    else
	strcpy(filetype, "text/plain");
}  
/* $end serve_static */

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
/* $begin serve_dynamic */
void serve_dynamic(int fd, char *filename, char *cgiargs) 
{
    char buf[MAXLINE], *emptylist[] = { NULL };

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); 
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));
  
    if (fork() == 0) { /* child */ //line:netp:servedynamic:fork
	/* Real server would set all CGI vars here */
	setenv("QUERY_STRING", cgiargs, 1); //line:netp:servedynamic:setenv
	dup2(fd, STDOUT_FILENO);         /* Redirect stdout to client */ //line:netp:servedynamic:dup2
	execve(filename, emptylist, environ); /* Run CGI program */ //line:netp:servedynamic:execve
    }

    if (wait(NULL) < 0) {
	fprintf(stderr, "Wait Error.\n");
	exit(1);
    }
    //wait(NULL); /* Parent waits for and reaps child */ //line:netp:servedynamic:wait
}
/* $end serve_dynamic */
