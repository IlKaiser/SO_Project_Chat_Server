
// macros for handling errors
#define handle_error_en(en, msg)    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define handle_error(msg)           do { perror(msg); exit(EXIT_FAILURE); } while (0)

/* Configuration parameters */
#define AWS             1
#if AWS
    #define SERVER_ADDRESS  "93.151.144.221" //AWS
#else
    #define SERVER_ADDRESS  "127.0.0.1" //AWS
#endif
#define MAX_QUEUE_LEN 1024

//Struct define
typedef struct handler_args_m
{
    int socket_desc;
} handler_args_m;
typedef struct input_m { 
    long mesg_type; 
    char mesg_text[MAX_QUEUE_LEN]; 
} input_m;
typedef struct handler_args_u{
    GtkWidget * view;
}handler_args_u;
struct user_par{
    GtkWidget* username;
    GtkWidget* password;
    //GtkWidget* dialog;
    GtkWidget* window;
    GtkApplication* app;
    int socket_desc;
} user_par;


//client thread
void* client(void *arg);
//manages incoming messages
void* thread_reciver(void *arg);
//gtk thread
static void activate (GtkApplication *app,gpointer data);
//GtkWidget* activate_login(GtkApplication *app);
void main_page(GtkApplication *app,gpointer user);
//login checker 
void login( GtkWidget *widget,gpointer data );
//update gtk thread
void* update (void* arg);

//close db connection
//static void exit_nicely(PGconn *conn, PGresult   *res);

//forces quit to server
void force_quit();
void handle_sigint(int sig);
