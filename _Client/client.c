#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <pthread.h>

#include "client.h"

///TODO:LOGIN NEL CLIENT

int main(int argc, char* argv[]) {
    int ret,bytes_sent,recv_bytes;
    char username[32];
    if (argv[1]!=NULL){
        strcpy(username,argv[1]);
        strcat(username,"\n");
    }
    else{
        strcpy(username,"client\n");
    }
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

    char ack[15];
    size_t ack_len = sizeof(ack);
    memset(ack, 0, ack_len);
    recv_bytes = 0;
    do {
        ret = recv(socket_desc, ack + recv_bytes, 1, 0);
        if (ret == -1 && errno == EINTR) continue;
        if (ret == -1) handle_error("Cannot read from the socket");
        if (ret == 0) handle_error_en(0xDEAD,"server is offline");
    } while ( ack[recv_bytes++] != '\n' );
    
    printf("l'ack è: %s\n, recv_bytes %d\n", ack,recv_bytes);
    fflush(stdout);

    if(strcmp(ack,ERROR_MSG)==0){
        handle_error_en(-1,"recived ERROR_MSG");
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
            if (ret == 0) handle_error_en(0xDEAD,"server is offline");
            //recv_bytes += ret;
        } while ( buf[recv_bytes++] != '\0' );
        printf("il buffer è: %s\n", buf);
        fflush(stdout);

        //check if recived errore msg from server
        if (strcmp(buf,ALONE_MSG)==0){
            printf("you are alone\n");
        }
    }while (strcmp(buf,ALONE_MSG)==0);

    ///TODO:input numero di return
    printf("select a username with his number: ");
    memset(buf,0,buf_len);
    if (fgets(buf, sizeof(buf), stdin) != (char*)buf) {
        fprintf(stderr, "Error while reading from stdin, exiting...\n");
        exit(EXIT_FAILURE);
    }
    ///TODO: manda il numero scelto
    strcat(buf,"\n");
    int pick_len = strlen(buf);
    printf("Pick_len %d\n",pick_len);
    // send message to server
    bytes_sent=0;
    while ( bytes_sent < pick_len) {
        ret = send(socket_desc, buf + bytes_sent, pick_len - bytes_sent, 0);
        if (ret == -1 && errno == EINTR) continue;
        if (ret == -1) handle_error("Cannot write to the socket");memset(ack, 0, ack_len);
        bytes_sent+=ret;
    }
    

    ///TODO: riceve l'ack e entra nel loop
    memset(ack, 0, ack_len);
    recv_bytes = 0;
    do {
        ret = recv(socket_desc, ack + recv_bytes,1, 0);
        if (ret == -1 && errno == EINTR) continue;
        if (ret == -1) handle_error("Cannot read from the socket");
        if (ret == 0) handle_error_en(0xDEAD,"server is offline");
	   //recv_bytes += ret;
    } while (ack[recv_bytes++]!='\n');
    printf("l'ack 2 è: %s, recv_bytes %d\n", ack,recv_bytes);
    fflush(stdout);
    if(strcmp(ack,ERROR_MSG)==0){
        //close connection with the server
        handle_error_en(-1,"recived ERROR_MSG");
        ret = close(socket_desc);
        if(ret) handle_error("Cannot close socket");
    }

    ///TODO: creare il thread di recive(per ricevere messaggi solo dal numero che hai selezionato async)
    pthread_t thread;

    // prepare arguments for the new thread
    handler_args_m *thread_args = malloc(sizeof(handler_args_m));
    thread_args->socket_desc = socket_desc;
    ret = pthread_create(&thread, NULL, thread_reciver, (void *)thread_args);
    if (ret) handle_error_en(ret, "Could not create a new thread");
    
    if (DEBUG) fprintf(stderr, "New thread created to handle the request!\n");
    
    ret = pthread_detach(thread); // I won't phtread_join() on this thread
    if (ret) handle_error_en(ret, "Could not detach the thread");
    

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
void* thread_reciver(void *arg){
    handler_args_m *args = (handler_args_m *)arg;
    int socket_desc = args->socket_desc;
    char buf1[1024];
    int ret;
    while(1){
        int recv_bytes=0;
        memset(buf1,0,sizeof(buf1));
        do {
            ret = recv(socket_desc, buf1 + recv_bytes,1, 0);
            if (ret == -1 && errno == EINTR) continue;
            if (ret == -1) handle_error("Cannot read from the socket");
            if (ret == 0) handle_error_en(0xDEAD,"server is offline");
        } while (buf1[recv_bytes++]!='\0');
        fprintf(stderr,"\nmessaggio di: %s", buf1);
    }

    return NULL;
}
