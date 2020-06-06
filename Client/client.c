#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>

#include "client.h"

int main(int argc, char* argv[]) {
    int ret,bytes_sent,recv_bytes;

    // variables for handling a socket
    int socket_desc;
    struct sockaddr_in server_addr = {0}; // some fields are required to be filled with 0

    // create a socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_desc < 0) handle_error("Could not create socket");

    // set up parameters for the connection
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(SERVER_PORT); // don't forget about network byte order!

    // initiate a connection on the socket
    ret = connect(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
    if(ret) handle_error("Could not create connection");

    if (DEBUG) fprintf(stderr, "Connection established!\n");

    char buf[1024];
    size_t buf_len = sizeof(buf);
    int msg_len;
    memset(buf, 0, buf_len);

    // display welcome message from server
    ///TODO: riceve e stampa la lista delle connessioni
    recv_bytes = 0;
    do {
        ret = recv(socket_desc, buf + recv_bytes, buf_len - recv_bytes, 0);
        if (ret == -1 && errno == EINTR) continue;
        if (ret == -1) handle_error("Cannot read from the socket");
        if (ret == 0) break;
	   recv_bytes += ret;
    } while ( buf[recv_bytes-1] != '\n' );
    printf("%s", buf);

    ///TODO:input numero di return
    fgets(buf, sizeof(buf), stdin) != (char*)buf;
    ///TODO: manda il numero scelto

    ///TODO: riceve l'ack e entra nel loop
    // read message from server
	    recv_bytes = 0;
    	do {
            ret = recv(socket_desc, buf + recv_bytes, buf_len - recv_bytes, 0);
            if (ret == -1 && errno == EINTR) continue;
            if (ret == -1) handle_error("Cannot read from the socket");
	        if (ret == 0) break;
	        recv_bytes += ret;

	    } while ( buf[recv_bytes-1] != '\n' );
        printf("Server response: %s\n", buf); // no need to insert '\0'
    ///TODO: creare il thread di recive(per ricevere messaggi solo dal numero che hai selezionato async)

    // main loop
    while (1) {
        char* quit_command = SERVER_COMMAND;
        size_t quit_command_len = strlen(quit_command);

        printf("Insert your message: ");

        /* Read a line from stdin
         *
         * fgets() reads up to sizeof(buf)-1 bytes and on success
         * returns the first argument passed to it. */
        if (fgets(buf, sizeof(buf), stdin) != (char*)buf) {
            fprintf(stderr, "Error while reading from stdin, exiting...\n");
            exit(EXIT_FAILURE);
        }

        msg_len = strlen(buf);
        buf[strlen(buf) - 1] = '\n'; // remove '\n' from the end of the message
        // send message to server
        bytes_sent=0;
        while ( bytes_sent < msg_len) {
            ret = send(socket_desc, buf + bytes_sent, msg_len - bytes_sent, 0);
            if (ret == -1 && errno == EINTR) continue;
            if (ret == -1) handle_error("Cannot write to the socket");
            bytes_sent += ret;
        }

        /* After a quit command we won't receive any more data from
         * the server, thus we must exit the main loop. */
        if (msg_len == quit_command_len && !memcmp(buf, quit_command, quit_command_len)) break;

        
    }


    // close the socket
    ret = close(socket_desc);
    if(ret) handle_error("Cannot close socket");

    if (DEBUG) fprintf(stderr, "Exiting...\n");

    exit(EXIT_SUCCESS);
}
///TODO: nuovo tread per le recive

