CC=gcc
CFLAGS= -Wall -g -O2 


all: server
server.o: server.c server.h
	$(CC) -c -o $@ $< $(CFLAGS)
server: server.o
		  $(CC) -o server server.o -lpthread
clean:
		rm -rf *.o	
