CFLAGS= -m64 -Wall -g -O2 
OS := $(shell uname)
ifeq ($(OS),Darwin)
	CC=gcc-9 
	CFLAGS += -I/usr/local/opt/libpq/include
else 
	CC=gcc
endif
<<<<<<< HEAD

=======
CFLAGS= -Wall -g -O2 
>>>>>>> 98071c1652d6286cc9e3e3282949c0d05a821ca9
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
