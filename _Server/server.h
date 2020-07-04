
// macros for handling errors
#define handle_error_en(en, msg)    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define handle_error(msg)           do { perror(msg); exit(EXIT_FAILURE); } while (0)

/* Configuration parameters */
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

// next postion setter
void set_next_position();
int get_position(int socket);
int is_occupied(int socket,int target);

// set disconnect into socket array
void set_disconnected(int socket_desc);

/// Server disconnection handler
void disconnection_handler(int socket_desc);

//check credentials in db
int login(char* credentials,int socket_desc);