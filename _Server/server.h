
// macros for handling errors
#define handle_error_en(en, msg)    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define handle_error(msg)           do { perror(msg); exit(EXIT_FAILURE); } while (0)

/* Configuration parameters */
#define DEBUG           1   // display debug messages
#define MAX_CONN_QUEUE  3   // max number of connections the server can queue
#define SERVER_ADDRESS  "127.0.0.1" //AWS
#define SERVER_COMMAND  "QUIT\n"
#define MAX_SIZE        128
#define SERVER_PORT     2015
#define ERROR_MSG       "0xAFFAF\n"
#define DISCONNECTED    0xAFFAF
#define ALONE_MSG       "Alone\n"
#define OK_MSG          "OK\n"
#define MSG_MSG         "H_MSG\n"
#define LIST_COMMAND    "_LIST_\n"

#define SEM_NAME        "/server"
#define AWS             0

//Struct define
typedef struct handler_args_s
{
    int socket_desc;
    struct sockaddr_in* client_addr;
} handler_args_t;


//Functions prototypes
//main threads
void mthreadServer(int server_desc) ;
void *thread_connection_handler(void *arg);
void connection_handler(int socket_desc, struct sockaddr_in* client_addr);

// function that formats the user list that server sends
void list_formatter(char buf[],int socket_desc);

//function that sends message to reciver
void send_mess(int index,char buff[]);

//close db connection
static void exit_nicely(PGconn *conn, PGresult   *res,int socket_desc);

//disconneciotn handler
void disconnection_handler(int index);

// next postion setter
void set_next_position();

// set disconnect into socket array
void set_disconnected(int socket_desc);

//check credentials in db
int login(char* credentials,int socket_desc);