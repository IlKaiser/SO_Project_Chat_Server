#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>

#include "client.h"

///TODO:LOGIN NEL CLIENT

int main(int argc, char* argv[]) {
    int ret,bytes_sent,recv_bytes;
    char username[8]={'c','l','i','e','n','t','\n','\0'};

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

    //SENDS HIS USERNAME TO THE SERVER
    int usr_len = strlen(username);
    // send message to server
    bytes_sent=0;
    while ( bytes_sent < usr_len) {
        ret = send(socket_desc, username + bytes_sent, usr_len - bytes_sent, 0);
        if (ret == -1 && errno == EINTR) continue;
        if (ret == -1) handle_error("Cannot write to the socket");
        bytes_sent += ret;
    }

    char ack[8];
    size_t ack_len = sizeof(ack);
    memset(ack, 0, ack_len);
    recv_bytes = 0;
    do {
        ret = recv(socket_desc, ack + recv_bytes, 1, 0);
        if (ret == -1 && errno == EINTR) continue;
        if (ret == -1) handle_error("Cannot read from the socket");
        if (ret == 0) break;
	   recv_bytes += ret;
    } while ( ack[recv_bytes-1] != '\n' );
    printf("%s", ack);
    if(strcmp(ack,ERROR_MSG)==0){
        //close connection with the server
        ret = close(socket_desc);
        if(ret) handle_error("Cannot close socket");
    }


    char buf[1024];
    size_t buf_len = sizeof(buf);
    // DISPLAYS LIST OF LOGGED USERNAMES
    ///TODO: riceve e stampa la lista delle connessioni
    do{
        memset(buf, 0, buf_len);
        recv_bytes = 0;
        do {
            ret = recv(socket_desc, buf + recv_bytes, 1, 0);
            if (ret == -1 && errno == EINTR) continue;
            if (ret == -1) handle_error("Cannot read from the socket");
            if (ret == 0) break;
            recv_bytes += ret;
        } while ( buf[recv_bytes-1] != '\n' );
        printf("il buffer è: %s\n", buf);

        //check if recived errore msg from server
        if (strcmp(buf,ALONE_MSG)==0){
            printf("you are alone\n");
        }
    }while (strcmp(buf,ALONE_MSG)==0);

    ///TODO:input numero di return
    printf("select a username with his number: ");
   if (fgets(buf, sizeof(buf), stdin) != (char*)buf) {
        fprintf(stderr, "Error while reading from stdin, exiting...\n");
        exit(EXIT_FAILURE);
    }
    int pick = atoi(buf);
    ///TODO: manda il numero scelto
    int pick_len = sizeof(pick);
    // send message to server
    bytes_sent=0;
    while ( bytes_sent < pick_len) {
        ret = send(socket_desc, &pick + bytes_sent, pick_len - bytes_sent, 0);
        if (ret == -1 && errno == EINTR) continue;
        if (ret == -1) handle_error("Cannot write to the socket");
        bytes_sent += ret;
    }



    ///TODO: riceve l'ack e entra nel loop
    memset(ack, 0, ack_len);
    recv_bytes = 0;
    do {
        ret = recv(socket_desc, ack + recv_bytes, ack_len - recv_bytes, 0);
        if (ret == -1 && errno == EINTR) continue;
        if (ret == -1) handle_error("Cannot read from the socket");
        if (ret == 0) break;
	   recv_bytes += ret;
    } while ( ack[recv_bytes-1] != '\n' );
    printf("%s", ack);
    if(strcmp(ack,ERROR_MSG)==0){
        //close connection with the server
        ret = close(socket_desc);
        if(ret) handle_error("Cannot close socket");
    }

    ///TODO: creare il thread di recive(per ricevere messaggi solo dal numero che hai selezionato async)

    // main loop
    int msg_len;
    while (1) {
        char* quit_command = SERVER_COMMAND;
        size_t quit_command_len = strlen(quit_command);

        printf("Insert your message: ");

        /* Read a line from stdin
         *
         * fgets() reads up to sizeof(buf)-1 bytes and on success
         * returns the first argument passed to it. */
        memset(buf,0,buf_len);
        if (fgets(buf, sizeof(buf), stdin) != (char*)buf) {
            fprintf(stderr, "Error while reading from stdin, exiting...\n");
            exit(EXIT_FAILURE);
        }
        //strcpy(buf,"ciao come va\n");
        printf("hai scritto: %s",buf);

        msg_len = strlen(buf);
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
