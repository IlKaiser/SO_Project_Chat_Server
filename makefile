
OS := $(shell uname)
ifeq ($(OS),Darwin)
	CC=gcc-9
else 
	CC=gcc
endif
CFLAGS= -m64 -Wall -g -O2 
SERVERFOLDER=_Server

all: server
server.o: $(SERVERFOLDER)/server.c $(SERVERFOLDER)/server.h
	$(CC) -c -o $@ $< $(CFLAGS)
server: server.o
		  $(CC) -o server server.o -lpthread
clean:
		rm -rf *.o	
