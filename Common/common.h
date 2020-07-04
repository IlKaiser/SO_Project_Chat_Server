#pragma once
// macros for handling errors
#define handle_error_en(en, msg)    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define handle_error(msg)           do { perror(msg); exit(EXIT_FAILURE); } while (0)

/* Configuration parameters */
#define DEBUG           1   // display debug messages
#define MAX_CONN_QUEUE  3   // max number of connections the server can queue

#define SERVER_COMMAND  "_QUIT_\n"
#define MAX_SIZE        128
#define SERVER_PORT     2015
#define ERROR_MSG       "0xAFFAF\n"
#define DISCONNECTED    0xAFFAF
#define ALONE_MSG       "Alone\n"
#define OK_MSG          "OK\n"
#define MSG_MSG         "H_MSG\n"
#define LIST_COMMAND    "_LIST_\n"
#define ERROR_OFF       0xDEAD


//Utility functions
void trim (char *dest, char *src);
int send_msg (int socket_desc,char* buf,int buf_size,char is_server);
int recive_msg (int socket_desc,char* buf,int buf_size,char is_server);

