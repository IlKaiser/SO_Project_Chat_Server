
OS := $(shell uname)
ifeq ($(OS),Darwin)
	CC=gcc-9
else 
	CC=gcc
endif
CFLAGS= -Wall -g -O2 
SERVERFOLDER=_Server
CLIENTFOLDER=_Client

GTK1= `pkg-config --cflags gtk+-3.0`
GTK2=`pkg-config --libs gtk+-3.0`




all: server 
server.o: $(SERVERFOLDER)/server.c $(SERVERFOLDER)/server.h
	$(CC) -c -o $@ $< $(CFLAGS)
server: server.o
		  $(CC) -o server server.o -lpthread -lpq

clean:
		rm -rf *.o	
