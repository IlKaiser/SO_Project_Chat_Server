#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <gtk/gtk.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "client.h"

///TODO:LOGIN NEL CLIENT

int main(int argc, char* argv[]) {
    
    //GTK init
    GtkApplication *app;
    int status;
    /*char* input_n;
    char *name = (char *) malloc(1 + strlen("test.")+ strlen(argv[1]) );
    input_n = argv[1];
    strcpy(name, "test.");
    strcat(name,input_n);*/
    app = gtk_application_new (argv[1]/*name*/, G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (activate),&argv[1]);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);
    return status;
}


void* thread_reciver(void *arg){
    handler_args_m *args = (handler_args_m *)arg;
    int socket_desc = args->socket_desc;
    char buf1[1024];
    int ret;

    //creating the message queue between client thread anch gtk thread
    key_t key; 
    int msgid;

    key = ftok("msg_queue", 65); 
    // msgget creates a message queue 
    // and returns identifier 
    msgid = msgget(key, 0666 | IPC_CREAT); 
    message.mesg_type = 1;

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
        // msgsnd to send message
        memset(message.mesg_text,0,sizeof(message.mesg_text));
        strcpy(message.mesg_text,buf1);
        msgsnd(msgid, &message, sizeof(message), 0);
    }

    return NULL;
}

void* client(void* arg){

    //creating the message queue between client thread anch gtk thread for output
    key_t key; 
    int msgid;

    key = ftok("msg_queue", 65); 
    // msgget creates a message queue 
    // and returns identifier 
    msgid = msgget(key, 0666 | IPC_CREAT); 
    message.mesg_type = 1; 

    //creating the message queue between client thread anch gtk thread for input
    key_t key1; 
    int msgid1; 
  
    // ftok to generate unique key 
    key1 = ftok("input_queue", 65); 
  
    // msgget creates a message queue 
    // and returns identifier 
    msgid1 = msgget(key1, 0666 | IPC_CREAT); 
    


    int ret,bytes_sent,recv_bytes;
    char username[32];
    char** argv=(char**)arg;
    if (*argv!=NULL){
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
    // msgsnd to send message
    memset(message.mesg_text,0,sizeof(message.mesg_text));
    strcpy(message.mesg_text,ack);
    msgsnd(msgid, &message, sizeof(message), 0);

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
        memset(message.mesg_text,0,sizeof(message.mesg_text));
        strcpy(message.mesg_text,buf);
        msgsnd(msgid, &message, sizeof(message), 0);

        //check if recived errore msg from server
        if (strcmp(buf,ALONE_MSG)==0){
            printf("you are alone\n");
        }
    }while (strcmp(buf,ALONE_MSG)==0);

    ///TODO:input numero di return
    printf("select a username with his number: ");
    memset(buf,0,buf_len);
    //if (fgets(buf, sizeof(buf), stdin) != (char*)buf) {
    msgrcv(msgid1, &input_m, sizeof(input_m), 1, 0);
    /*fprintf(stderr, "Error while reading from stdin, exiting...\n");
    exit(EXIT_FAILURE);*/
    ///TODO: manda il numero scelto
    strcpy(buf,input_m.mesg_text);
    //strcat(buf,"\n");
    int pick_len = strlen(buf);
    printf("Pick_len %d\n",pick_len);
    // send message to server
    bytes_sent=0;
    while ( bytes_sent < pick_len) {
        ret = send(socket_desc, buf + bytes_sent, pick_len - bytes_sent, 0);
        if (ret == -1 && errno == EINTR) continue;
        if (ret == -1) handle_error("Cannot write to the socket");
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
    memset(message.mesg_text,0,sizeof(message.mesg_text));
    strcpy(message.mesg_text,ack);
    msgsnd(msgid, &message, sizeof(message), 0);

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
        msgrcv(msgid1, &input_m, sizeof(input_m), 1, 0);
        /*fprintf(stderr, "Error while reading from stdin, exiting...\n");
        exit(EXIT_FAILURE);*/
        strcpy(buf,input_m.mesg_text);
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

    // to destroy the message queue 
    msgctl(msgid, IPC_RMID, NULL); 

    exit(EXIT_SUCCESS);
}
void* update (void* arg){
    handler_args_u* args = (handler_args_u*)arg;
    GtkWidget* view = args->view;

    key_t key; 
    int msgid; 
  
    // ftok to generate unique key 
    key = ftok("msg_queue", 65); 
  
    // msgget creates a message queue 
    // and returns identifier 
    msgid = msgget(key, 0666 | IPC_CREAT); 
    while (1){
        // msgrcv to receive message 
        msgrcv(msgid, &message, sizeof(message), 1, 0); 
    
        // display the message 
        printf("Data Received is : %s \n",  message.mesg_text);
        GtkTextBuffer* buffer = gtk_text_buffer_new(NULL);
        
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER (buffer),message.mesg_text,strlen(message.mesg_text));
        gtk_text_view_set_buffer(GTK_TEXT_VIEW (view),GTK_TEXT_BUFFER (buffer));
    }
   
    // to destroy the message queue 
    msgctl(msgid, IPC_RMID, NULL); 
    return NULL;
}
static void callback( GtkWidget *widget,gpointer data )
{
    if (data==NULL){
        return;
    }
    GtkWidget* id = data;
    char* input = (char*)gtk_entry_get_text(GTK_ENTRY(id));
    key_t key; 
    int msgid;

    key = ftok("msg_queue", 65); 
    // msgget creates a message queue 
    // and returns identifier 
    msgid = msgget(key, 0666 | IPC_CREAT); 
    input_m.mesg_type = 1;
    memset(input_m.mesg_text,0,sizeof(input_m.mesg_text));
    strcpy(input_m.mesg_text,input);
    msgsnd(msgid, &input_m, sizeof(input_m), 0);
}


static void activate (GtkApplication *app,gpointer user_data){
    
    int ret;
    pthread_t thread;
    char** argv=(char**)user_data;
    
    // prepare arguments for the new thread
    ret = pthread_create(&thread, NULL,client,argv);
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

    paned=gtk_paned_new(GTK_ORIENTATION_VERTICAL);

    /* The function directly below is used to add children to the scrolled window 
    * with scrolling capabilities (e.g text_view), otherwise, 
    * gtk_scrolled_window_add_with_viewport() would have been used
    */
    /* Here we construct the container that is going pack our buttons */
    grid = gtk_grid_new ();

    /* Pack the container in the window */
    //gtk_container_add (GTK_CONTAINER (window), scrolled_window);
    gtk_container_add (GTK_CONTAINER (window), paned);
    gtk_widget_set_size_request(paned,300,-1);
    gtk_paned_pack1 (GTK_PANED (paned), view,FALSE,FALSE);
    gtk_paned_add2 (GTK_PANED (paned), grid);



    /* Place the Quit button in the grid cell (0, 1), and make itvoid
    * span 2 columns.void
    */
    gtk_grid_attach(GTK_GRID (grid), scrolled_window,0,0,10,10);
    button = gtk_button_new_with_label ("Send");
    g_signal_connect (button, "clicked",G_CALLBACK (callback),im);

    gtk_grid_attach(GTK_GRID(grid),im,11,11,4,1);
    gtk_grid_attach (GTK_GRID (grid), button, 11,16,2,1);



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
