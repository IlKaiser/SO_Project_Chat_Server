CFLAGS= -m64 -Wall -g -O2 
OS := $(shell uname)
ifeq ($(OS),Darwin)
	CC=gcc-9 
	CFLAGS += -I/usr/local/opt/libpq/include
else 
	CC=gcc
endif

SERVERFOLDER=_Server
CLIENTFOLDER=_Client

GTK1= `pkg-config --cflags gtk+-3.0`
GTK2=`pkg-config --libs gtk+-3.0`




all: server client
server.o: $(SERVERFOLDER)/server.c $(SERVERFOLDER)/server.h
	$(CC) -c -o $@ $< $(CFLAGS)
server: server.o
		  $(CC) -o server server.o -lpthread -lpq  
client.o: $(CLIENTFOLDER)/client.c $(CLIENTFOLDER)/client.h
	$(CC) $(GTK1) -c -o $@ $<  $(CFLAGS)
client: client.o
		  $(CC) $(GTK2) -o client client.o  -lpthread 

clean:
		rm -rf *.o	
