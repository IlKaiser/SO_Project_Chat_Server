CFLAGS= -Wall -g -O2 
SERVERFOLDER=_Server


all: server 
common.o: Common/common.c Common/common.h
	$(CC) -c -o $@ $< $(CFLAGS)

all: server 
server.o: $(SERVERFOLDER)/server.c $(SERVERFOLDER)/server.h common.o
	$(CC) -c -o $@ $< $(CFLAGS)
server: server.o
		  $(CC) -o server server.o -lpthread -lpq

clean:
		rm -rf *.o	
