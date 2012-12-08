/* server.h */
#ifndef __SERVER_H__
#define __SERVER_H__

#include "rio.h"

#define THREADS 20
#define MAXLINE 8192
#define MAXBUF  8192

static void process_http(int fd);
int parse_uri(char *uri, char *filename, char *cgiargs);
void read_requesthdrs(rio_t *rp);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

#endif /* server.h */
