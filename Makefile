# 
# Makefile for webserver
# Nayef Copty and K Al Najar
# This makes use of -Wall, -Werror, and -Wmissing-prototypes

CC = gcc
CFLAGS = -Wall -Werror -Wmissing-prototypes
OBJS = server.o rio.o

all: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o sysstatd
	rm *.o

server.o: server.c
	$(CC) $(CFLAGS) -c server.c

rio.o: rio.c rio.h
	$(CC) $(CFLAGS) -c rio.c

clean:
	rm -f *~ *.o sysstatd