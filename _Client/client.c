#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <gtk/gtk.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <libpq-fe.h>


#include "client.h"


// message queues
int update_msg;
int input_msg;
int upin_msg;
input_m* input_str;
input_m* upin_str;
input_m* message;

//global variable for window manipulation
int main_window_id;
int socket_desc_copy;
GtkApplication *app;




struct msqid_ds buf;

int main(int argc, char* argv[]) {
    signal(SIGINT, handle_sigint); 
    input_str = (input_m*)malloc(sizeof(input_m));
    upin_str = (input_m*)malloc(sizeof(input_m));
    message = (input_m*)malloc(sizeof(input_m));
    int flags = 0666;

    key_t id1 =ftok(argv[0], 1);
    key_t id2 =ftok(argv[0], 2);
    key_t id3 =ftok(argv[0], 3);
    printf ("la chiave è :%d\n",id1);
    printf ("la chiave è :%d\n",id2);
    printf ("la chiave è :%d\n",id3);

    msgctl(msgget(id1, 0), IPC_RMID, NULL);
    msgctl(msgget(id2, 0), IPC_RMID, NULL);
    msgctl(msgget(id3, 0), IPC_RMID, NULL);
    update_msg = msgget(id1, flags| IPC_CREAT);
    input_msg = msgget(id2, flags| IPC_CREAT);
    upin_msg = msgget(id3, flags| IPC_CREAT);

    input_str->mesg_type = 1;
    upin_str->mesg_type = 1;
    message->mesg_type = 1;

    //GTK init
    int status;
    
    //GtkApplication *app;
    app = gtk_application_new (argv[1]/*name*/, G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (activate),&argv[1]);
    status = g_application_run (G_APPLICATION (app),argc, argv);
    //g_object_unref (app);

    return status;
}


void* thread_reciver(void *arg){
    handler_args_m *args = (handler_args_m *)arg;
    int socket_desc = args->socket_desc;
    char buf1[1024];
    int ret;

    // msgget creates a message queue 
    // and returns identifier
    #if DEBUG
    printf("thread reciver msg\n: %s\n",message->mesg_text);
    #endif

    while(1){
        int recv_bytes=0;
        memset(buf1,0,sizeof(buf1));
        do {                                
            ret = recv(socket_desc, buf1 + recv_bytes,1, 0);
            if (ret == -1 && errno == EINTR) continue;
            if (ret == -1) handle_error("Cannot read from the socket");
            if (ret == 0) handle_error_en(0xDEAD,"server is offline");
        } while (buf1[recv_bytes++]!='\0');
        fprintf(stderr,"\nmessaggio di\n: %s", buf1);
        // msgsnd to send message
        message->mesg_type = 1;
        memset(message->mesg_text,0,sizeof(message->mesg_text));
        strcpy(message->mesg_text,buf1);
        msgsnd(update_msg, message, sizeof(message), 0);
        #if DEBUG
        printf("thread reciver msg snd\n: %s",message->mesg_text);
        #endif
    }

    msgctl(update_msg, IPC_RMID, NULL); 
    return NULL;
}

void* client(void* arg){
    
    int ret,bytes_sent,recv_bytes;
    char* user__name = arg;
    char username[32];
    strcat(user__name,"\n");
    strcpy(username,user__name);

    #if DEBUG
        printf("Username got %s",username);
    #endif
    
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

    GtkWindow* main_window=gtk_application_get_window_by_id(app,main_window_id);

    socket_desc_copy=socket_desc;

    g_signal_connect(G_OBJECT(main_window), "destroy", G_CALLBACK(force_quit),NULL);

    //SENDS HIS USERNAME TO THE SERVER
    int usr_len = strlen(username);
    printf("Usr len:%d, username %s\n ",usr_len,username);
    // send message to server
    bytes_sent=0;
    while ( bytes_sent < usr_len) {
        ret = send(socket_desc, username + bytes_sent, usr_len - bytes_sent, 0);
        if (ret == -1 && errno == EINTR) continue;
        if (ret == -1) handle_error("Cannot write to the socket");
        bytes_sent += ret;
        printf("Sent %s, Bytes sent %d\n",username,ret);
    }

    #if DEBUG
        printf("Username sent\n");
    #endif

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
    // msgsnd to send message
    memset(message->mesg_text,0,sizeof(message->mesg_text));
    strcpy(message->mesg_text,ack);
    msgsnd(update_msg, message, sizeof(message), 0);
    #if DEBUG
    printf("thread client primo ack msg\n: %s",message->mesg_text);
    #endif

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
        printf("la lista è:\n %s", buf);
        fflush(stdout);
        if (!(strcmp(buf,ALONE_MSG)==0)){
            memset(message->mesg_text,0,sizeof(message->mesg_text));
            strcpy(message->mesg_text,buf);
            msgsnd(update_msg, message, sizeof(message), 0);
            #if DEBUG
                printf("thread client lista msg\n: %s",message->mesg_text);
            #endif
        }
        else{
            printf("you are alone\n");
        }
        

        //check if recived errore msg from server
    }while (strcmp(buf,ALONE_MSG)==0);

    ///TODO:input numero di return
    //printf("select a username with his number: ");
    memset(buf,0,buf_len);
    //if (fgets(buf, sizeof(buf), stdin) != (char*)buf) {
    msgrcv(input_msg, input_str, sizeof(input_m), 1, 0);
    #if DEBUG
    printf("thread client input msg : %s\n",input_str->mesg_text);
    #endif
    /*fprintf(stderr, "Error while reading from stdin, exiting...\n");
    exit(EXIT_FAILURE);*/
    ///TODO: manda il numero scelto
    printf("scelta nel msg è: %s\n",input_str->mesg_text);
    strcpy(buf,input_str->mesg_text);
    strcat(buf,"\n");
    //strcat(buf,"\n");
    int pick_len = strlen(buf);
    printf("Pick_len %d\n",pick_len);
    printf("Pick: %s\n",buf);
    // send message to server
    bytes_sent=0;
    while ( bytes_sent < pick_len) {
        ret = send(socket_desc, buf + bytes_sent, pick_len - bytes_sent, 0);
        if (ret == -1 && errno == EINTR) continue;
        if (ret == -1) handle_error("Cannot write to the socket");
        bytes_sent+=ret;
    }
    printf("inviata la scelta\n");
    fflush(stdout);

    

    ///TODO: riceve l'ack e entra nel loop
    memset(ack, 0, ack_len);
    recv_bytes = 0;

    printf("aspetto ack\n");
    fflush(stdout);
    do {
        ret = recv(socket_desc, ack + recv_bytes,1, 0);
        if (ret == -1 && errno == EINTR) continue;
        if (ret == -1) handle_error("Cannot read from the socket");
        if (ret == 0) handle_error_en(0xDEAD,"server is offline");
	   //recv_bytes += ret;
    } while (ack[recv_bytes++]!='\n');
    printf("l'ack 2 è\n: %s, recv_bytes %d\n", ack,recv_bytes);
    fflush(stdout);
    memset(message->mesg_text,0,sizeof(message->mesg_text));
    strcpy(message->mesg_text,ack);
    msgsnd(update_msg, message, sizeof(message), 0);

    #if DEBUG
    printf("thread client secondo ack msg\n: %s",message->mesg_text);
    #endif

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
        //if (fgets(buf, sizeof(buf), stdin) != (char*)buf) {
        msgrcv(input_msg, input_str, sizeof(input_m), 1, 0);

        #if DEBUG
        printf("thread client input msg tuo msg: %s",input_str->mesg_text);
        #endif
        /*fprintf(stderr, "Error while reading from stdin, exiting...\n");
        exit(EXIT_FAILURE);*/
        strcpy(buf,input_str->mesg_text);
        strcat(buf,"\n");
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
    msgctl(update_msg, IPC_RMID, NULL);
    msgctl(input_msg, IPC_RMID, NULL);
    msgctl(upin_msg, IPC_RMID, NULL);
    free(input_str);
    free(upin_str);
    free(message);
    
    exit(EXIT_SUCCESS);
}
void* update (void* arg){
    handler_args_u* args = (handler_args_u*)arg;
    GtkWidget* view = args->view;

    GtkTextIter iter;
    while (1){
        // msgrcv to receive message 
        msgctl(update_msg, IPC_STAT, &buf);
        if (buf.msg_qnum > 0){
            
            msgrcv(update_msg, message, sizeof(message), 1, 0);
            #if DEBUG
                printf("thread update msg\n: %s\n",message->mesg_text);
            #endif
            // display the message 
            printf("Data Received is \n: %s \n",  message->mesg_text);
            GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
            gtk_text_buffer_get_end_iter (buffer,&iter);
            
            gtk_text_buffer_insert(GTK_TEXT_BUFFER (buffer),&iter,message->mesg_text,strlen(message->mesg_text));
            gtk_text_view_set_buffer(GTK_TEXT_VIEW (view),GTK_TEXT_BUFFER (buffer));
        }
        msgctl(upin_msg, IPC_STAT, &buf);
        if (buf.msg_qnum>0){
            msgrcv(upin_msg, upin_str, sizeof(input_m), 1, 0);
            #if DEBUG
                printf("thread update msg\n: %s\n",upin_str->mesg_text);
            #endif
            // display the message 
            printf("Data sent is \n: %s \n",  upin_str->mesg_text);
            GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
            gtk_text_buffer_get_end_iter (buffer,&iter);
            gtk_text_buffer_insert(GTK_TEXT_BUFFER (buffer),&iter,upin_str->mesg_text,strlen(upin_str->mesg_text));
            gtk_text_view_set_buffer(GTK_TEXT_VIEW (view),GTK_TEXT_BUFFER (buffer));
        }
    }
    return NULL;
}
static void callback( GtkWidget *widget,gpointer data )
{
    if (data==NULL){
        return;
    }
    GtkWidget* id = data;
    char* input = (char*)gtk_entry_get_text(GTK_ENTRY(id));

    
    memset(input_str->mesg_text,0,sizeof(input_str->mesg_text));
    memset(upin_str->mesg_text,0,sizeof(input_str->mesg_text));
    strcpy(input_str->mesg_text,input);
    strcpy(upin_str->mesg_text,input);
    printf("input: %s",input);
    printf("msg: %s",input_str->mesg_text);
    msgsnd(input_msg, input_str, sizeof(input_m),0);
    msgsnd(upin_msg, upin_str, sizeof(input_m), 0);
    gtk_entry_set_text(GTK_ENTRY(id),"");
}


static void exit_nicely(PGconn *conn, PGresult   *res)
{
    PQclear(res);
    PQfinish(conn);
    exit(1);
}
void main_page(GtkApplication *app,char* user){
    //AFTER LOGIN CODE
    int ret;
    pthread_t thread;
    #if DEBUG
        printf("il tuo nome è: %s\n",user);
    #endif
    // prepare arguments for the new thread
    ret = pthread_create(&thread, NULL,client,user);
    if (ret) handle_error_en(ret, "Could not create a new thread");
    
    if (DEBUG) fprintf(stderr, "New thread created to handle the request!\n");
    
    ret = pthread_detach(thread); // I won't phtread_join() on this thread
    if (ret) handle_error_en(ret, "Could not detach the thread");
    
    GtkWidget *window;
    GtkWidget *paned;
    GtkWidget *grid;
    GtkWidget *button;
    GtkWidget *im;
    GtkWidget *scrolled_window;
    GtkWidget *view;
    
    

    /* create a new window, and set its title */
    window = gtk_application_window_new (app);
    gtk_window_set_title (GTK_WINDOW (window), "Window");
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    gtk_window_set_default_size(GTK_WINDOW(window),400,400);
    main_window_id=gtk_application_window_get_id(GTK_APPLICATION_WINDOW(window));


    im=gtk_entry_new ();


    view=gtk_text_view_new();
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), GTK_WRAP_WORD); 
    gtk_text_view_set_editable (GTK_TEXT_VIEW(view),FALSE);
    gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW(view),FALSE);
    //gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(view),GTK_TEXT_WINDOW_TEXT);GTK_TEXT_WINDOW_TEXT



    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), 
                                    GTK_POLICY_AUTOMATIC, 
                                    GTK_POLICY_AUTOMATIC);

    gtk_container_add (GTK_CONTAINER (scrolled_window),view);

    paned=gtk_paned_new(GTK_ORIENTATION_VERTICAL);

    /* The function directly below is used to add children to the scrolled window 
    * with scrolling capabilities (e.g text_view), otherwise, 
    * gtk_scrolled_window_add_with_viewport() would have been used
    */
    /* Here we construct the container that is going pack our buttons */
    grid = gtk_grid_new ();

    /* Pack the container in the window */
    //gtk_container_add (GTK_CONTAINER (window), scrolled_window);
    gtk_paned_add1 (GTK_PANED (paned),scrolled_window);
    gtk_paned_add2 (GTK_PANED (paned), grid);
    gtk_container_add (GTK_CONTAINER (window), paned);
    gtk_widget_set_size_request(paned,300,-1);
    



    /* Place the Quit button in the grid cell (0, 1), and make itvoid
    * span 2 columns.void
    */
    //gtk_grid_attach(GTK_GRID (grid), scrolled_window,0,0,10,10);
    button = gtk_button_new_with_label ("Send");
    g_signal_connect (button, "clicked",G_CALLBACK (callback),im);

    gtk_grid_attach(GTK_GRID(grid),im,0,0,4,1);
    gtk_grid_attach (GTK_GRID (grid), button, 0,5,2,1);



    /* Now that we are done packing our widgets, we show them all
    * in one go, by calling gtk_widget_show_all() on the window.
    * This call recursively calls gtk_widget_show() on all widgets
    * that are contained in the window, directly or indirectly.
    */
    gtk_widget_show_all (window);

    // prepare arguments for the new thread
    handler_args_u* arg_up = (handler_args_u*)malloc(sizeof(handler_args_u));
    arg_up->view=view; 

    ret = pthread_create(&thread, NULL,update,arg_up);
    if (ret) handle_error_en(ret, "Could not create a new thread");
    
    if (DEBUG) fprintf(stderr, "New thread created to handle the request!\n");
    
    ret = pthread_detach(thread); // I won't phtread_join() on this thread
    if (ret) handle_error_en(ret, "Could not detach the thread");
    
}
void login( GtkWidget *widget,gpointer data ){
    struct user_par *arg = data; 
    GtkWidget* window = arg->window;
    GtkWidget* dialog; //= arg->dialog;
    GtkWidget* user = arg->username;
    GtkWidget* pas = arg->password;
    GtkApplication* app = arg->app;
    
    char* i_user = (char*)gtk_entry_get_text(GTK_ENTRY(user));
    char* i_pas = (char*)gtk_entry_get_text(GTK_ENTRY(pas));

    //connetto al db
    const char *conninfo = "hostaddr=15.236.174.17 port=5432 dbname=SO_chat user=postgres password=Quindicimaggio_20 sslmode=disable";
    PGconn *conn;
    PGresult *res;
    

    conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    }
    printf("hai inserito: %s ,%s \n",i_user,i_pas);
    const char* paramValue[2] = {i_user,i_pas};
    res = PQexecParams(conn,
                    "select username from users where username=$1 and password=$2",
                    2,       /* two param */
                    NULL,    /* let the backend deduce param type */
                    paramValue,
                    NULL,
                    NULL,
                    1);      /* ask for binary results */
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        exit_nicely(conn,res);
    }
    int ris=PQntuples(res);
    printf("risposta del db: %d\n",ris);
    if (ris==1){
        gtk_window_close(GTK_WINDOW(window));
        main_page(app,i_user);
    }
    else{
        dialog = gtk_message_dialog_new(GTK_WINDOW (window),GTK_DIALOG_DESTROY_WITH_PARENT , GTK_MESSAGE_INFO,GTK_BUTTONS_NONE, "NOME UTENTE O PASSWORD SBAGLIATE" );
        gtk_dialog_run(GTK_DIALOG(dialog));

    }

}
static void activate (GtkApplication *app){

    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *button;
    GtkWidget *in_user;
    GtkWidget *in_pass;
    //GtkWidget *dialog;
    GtkWidget * usr_label;
    GtkWidget * pas_label;


    window = gtk_application_window_new (app);
    gtk_window_set_title (GTK_WINDOW (window), "Window");
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    gtk_window_set_default_size(GTK_WINDOW(window),100,100);

    in_user=gtk_entry_new ();
    in_pass=gtk_entry_new ();
    gtk_entry_set_visibility(GTK_ENTRY(in_pass),FALSE); 
    usr_label=gtk_label_new("Username");
    pas_label=gtk_label_new("Password");

    //dialog = gtk_message_dialog_new(GTK_WINDOW (window),GTK_DIALOG_DESTROY_WITH_PARENT , GTK_MESSAGE_INFO, GTK_BUTTONS_OK , "NOME UTENTE O PASSWORD SBAGLIATE" );
    

    grid = gtk_grid_new ();

    button = gtk_button_new_with_label ("LOGIN");
    user_par.username=in_user;
    user_par.password=in_pass;
    //user_par.dialog=dialog;
    user_par.window=window;
    user_par.app=app;
    g_signal_connect (button, "clicked",G_CALLBACK (login),&user_par);

    
    gtk_grid_attach(GTK_GRID(grid),usr_label,3,1,4,1);
    gtk_grid_attach(GTK_GRID(grid),in_user,3,2,4,1);

    gtk_grid_attach(GTK_GRID(grid),pas_label,3,4,4,1);
    gtk_grid_attach(GTK_GRID(grid),in_pass,3,5,4,1);

    gtk_grid_attach(GTK_GRID(grid),button,3,7,4,1);


    gtk_container_add (GTK_CONTAINER (window),grid);
    gtk_widget_show_all (window);
}
void force_quit(){

    int ret,bytes_sent,msg_len;

    char buf[strlen(SERVER_COMMAND)+1];
    strcpy(buf,SERVER_COMMAND);
    msg_len = strlen(buf);


    int socket_desc=socket_desc_copy;
    
    // send message to server
    bytes_sent=0;
    while ( bytes_sent < msg_len) {
        ret = send(socket_desc, buf + bytes_sent, msg_len - bytes_sent, 0);
        if (ret == -1 && errno == EINTR) continue;
        if (ret == -1) handle_error("Cannot write to the socket");
        bytes_sent += ret;
    }

    msgctl(update_msg, IPC_RMID, NULL);
    msgctl(input_msg, IPC_RMID, NULL);
    msgctl(upin_msg, IPC_RMID, NULL);
    free(input_str);
    free(upin_str);
    free(message);
    exit(EXIT_SUCCESS);

}
void handle_sigint(int sig) 
{ 
    printf("Caught signal %d\n", sig); 
    force_quit();
} 