
// macros for handling errors
#define handle_error_en(en, msg)    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define handle_error(msg)           do { perror(msg); exit(EXIT_FAILURE); } while (0)

/* Configuration parameters */
#define DEBUG           1   // display debug messages
#define MAX_CONN_QUEUE  3   // max number of connections the server can queue
#define SERVER_ADDRESS  "21.255.185.235" //AWS
#define SERVER_COMMAND  "QUIT\n"
#define SERVER_PORT     2015
#define ERROR_MSG       "0xAFFAF\n"
#define ERROR_OFF       0xDEAD
#define OK_MSG          "OK\n"
#define ALONE_MSG       "Alone\n"
//Struct define
typedef struct handler_args_m
{
    int socket_desc;
} handler_args_m;

//manages incoming messages
void* thread_reciver(void *arg);
