# 
# Makefile for webserver
# Nayef Copty and K Al Najar
# This makes use of -Wall, -Werror, and -Wmissing-prototypes

CC = gcc
CFLAGS = -Wall -Werror -Wmissing-prototypes
OBJS = server.o rio.o list.o
LDLIBS = -lpthread

.INTERMEDIATE: $(OBJS)

all: sysstatd

sysstatd: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDLIBS) -o sysstatd -L thread-pool -lthreadpool

server.o: server.h server.c
rio.o: rio.h rio.c
list.o: list.h list.c

threadpool:
	cd thread-pool; make clean threadpool

clean:
	rm -f *~ *.o sysstatd