# 
# Makefile for webserver
# Nayef Copty and K Al Najar
# This makes use of -Wall, -Werror, and -Wmissing-prototypes

CC = gcc
CFLAGS = -Wall -Werror -Wmissing-prototypes
OBJS = server.o rio.o
LDLIBS = -lpthread

all: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDLIBS) -o sysstatd -L thread-pool -lthreadpool
	rm *.o

server.o: server.h server.c
	$(CC) $(CFLAGS) -c server.c

rio.o: rio.c rio.h
	$(CC) $(CFLAGS) -c rio.c

threadpool:
	cd thread-pool; make clean threadpool

clean:
	rm -f *~ *.o sysstatd