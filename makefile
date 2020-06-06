CC=gcc
CFLAGS= -Wall -g -O2 
SERVERFOLDER=Server

all: server
server.o: $(SERVERFOLDER)/server.c $(SERVERFOLDER)/server.h
	$(CC) -c -o $@ $< $(CFLAGS)
server: server.o
		  $(CC) -o server server.o -lpthread
clean:
		rm -rf *.o	
