CC=gcc
CFLAGS= -Wall -g -O2 
DEPS = server.h

all: server
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
server: server.o
		  $(CC) -o server server.o -lpthread
clean:
		rm -rf *.o	
