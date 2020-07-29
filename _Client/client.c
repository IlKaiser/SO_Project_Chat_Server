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
#include "../Common/common.h"


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



char* welcome_not_hacker =  "This is a\nCHAT SERVER\nBy Marco Calamo and Ghenadie Artic\n\n\nType QUIT to quit or type _LIST_ to get a new list of users\n\n\n";
char* WELCOME = "This is a\n"
    " ######  ##     ##    ###    ########     ######  ######## ########  ##     ## ######## ########  \n"
    "##    ## ##     ##   ## ##      ##       ##    ## ##       ##     ## ##     ## ##       ##     ## \n"
    "##       ##     ##  ##   ##     ##       ##       ##       ##     ## ##     ## ##       ##     ## \n"
    "##       ######### ##     ##    ##        ######  ######   ########  ##     ## ######   ########  \n"
    "##       ##     ## #########    ##             ## ##       ##   ##    ##   ##  ##       ##   ##   \n"
    "##    ## ##     ## ##     ##    ##       ##    ## ##       ##    ##    ## ##   ##       ##    ##  \n"
    " ######  ##     ## ##     ##    ##        ######  ######## ##     ##    ###    ######## ##     ## \n"
    "By Marco Calamo and Ghenadie Artic\n";

//copy of thread for closing in signal handler
pthread_t t_copy_reciver;
pthread_t t_copy_updater;

//struct to collect infos about update queue
struct msqid_ds info_update_queue;

int main(int argc, char* argv[]) {
    //init signal handler
    signal(SIGINT, handle_sigint); 

    //init message queues and queue structs
    input_str = (input_m*)malloc(sizeof(input_m));
    upin_str = (input_m*)malloc(sizeof(input_m));
    message = (input_m*)malloc(sizeof(input_m));
    int flags = 0666;

    key_t id1 =ftok(argv[0], 1);
    key_t id2 =ftok(argv[0], 2);
    key_t id3 =ftok(argv[0], 3);
    printf("%s",WELCOME);
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

    // 1. CONNECTION TO SERVER
    int ret;
    int socket_desc;
    struct sockaddr_in server_addr = {0}; // some fields are required to be filled with 0

    //1.1 create a socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_desc < 0) handle_error("Could not create socket");

    //1.2 set up parameters for the connection
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(SERVER_PORT); // don't forget about network byte order!

    //1.3 initiate a connection on the socket
    ret = connect(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
    if(ret) handle_error("Could not create connection");

    if (DEBUG) fprintf(stderr, "Connection established!\n");

    //1.4 GTK init
    int status;
    handler_args_m* socket = (handler_args_m*)malloc(sizeof(handler_args_m));
    socket->socket_desc=socket_desc;
    app = gtk_application_new (NULL, G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (activate),socket);
    status = g_application_run (G_APPLICATION (app),argc, argv);

    return status;
}

//Thread made to recive every message from server in async way
void* thread_reciver(void *arg){
    handler_args_m *args = (handler_args_m *)arg;
    int socket_desc = args->socket_desc;
    char buf[1024];
    char buf1[1024];

    while(1){
        memset(buf1,0,sizeof(buf1));
        recive_msg(socket_desc,buf1,sizeof(buf1),0);
        #if DEBUG
            fprintf(stderr,"\nmessage by\n: %s", buf1);
        #endif
        if(strcmp(buf1,ERROR_MSG)==0){
            //close connection with the server
            handle_error_en(-1,"recived ERROR_MSG");
            int ret = close(socket_desc);
            if(ret) handle_error("Cannot close socket");
        }
        
        if (!strcmp(buf1,MSG_MSG)){
            printf("waiting for messages\n");
            memset(buf,0,sizeof(buf));
            strcpy(buf,MSG_MSG);
            send_msg(socket_desc,buf,strlen(buf),0);

        }
        //send message to update thread
        if (strcmp(buf1,ALONE_MSG) && strcmp(buf1,MSG_MSG)){
            message->mesg_type = 1;
            memset(message->mesg_text,0,sizeof(message->mesg_text));
            strcpy(message->mesg_text,buf1);
            msgsnd(update_msg, message, sizeof(message), 0);
        }
    }
    return NULL;
}
//main thread for client
void* client(void* arg){
    
    int ret;
    handler_args_m* data_socket = arg;
    int socket_desc = data_socket->socket_desc;

    handler_args_m *thread_args = malloc(sizeof(handler_args_m));
    thread_args->socket_desc = socket_desc;
    pthread_t thread;

    // 1 Start thread reciver
    ret = pthread_create(&thread, NULL, thread_reciver, (void *)thread_args);
    t_copy_reciver = thread;
    if (ret) handle_error_en(ret, "Could not create a new thread");
    if (DEBUG) fprintf(stderr, "New thread_reciver created to handle the request!\n");

    //1.1 Setting signal handler on window closing
    GtkWindow* main_window=gtk_application_get_window_by_id(app,main_window_id);
    socket_desc_copy=socket_desc;
    g_signal_connect(G_OBJECT(main_window), "destroy", G_CALLBACK(force_quit),NULL);

    
    char buf[1024];
    size_t buf_len = sizeof(buf);
    // main loop
    int msg_len;
    while (1) {
        char* quit_command = SERVER_COMMAND;
        
        
        memset(buf,0,buf_len);
        msgrcv(input_msg, input_str, sizeof(input_m), 1, 0);
        
        strcpy(buf,input_str->mesg_text);
        strcat(buf,"\n");
        printf("hai scritto: %s\n",buf);

        msg_len = strlen(buf);
        // send message to server
        send_msg(socket_desc,buf,msg_len,0);

        if(!strcmp(buf,quit_command)){
            break;
        }
    }
    pthread_cancel(thread);

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
//thread to update gui on message recive
void* update (void* arg){
    handler_args_u* args = (handler_args_u*)arg;
    GtkWidget* view = args->view;

    GtkTextIter iter;
    while (1){
        //check if there is a message in update queue 
        msgctl(update_msg, IPC_STAT, &info_update_queue);
        if (info_update_queue.msg_qnum > 0){
            
            msgrcv(update_msg, message, sizeof(message), 1, 0);
            // display the message 
            GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
            gtk_text_buffer_get_end_iter (buffer,&iter);
            
            gtk_text_buffer_insert(GTK_TEXT_BUFFER (buffer),&iter,message->mesg_text,strlen(message->mesg_text));
            gtk_text_view_set_buffer(GTK_TEXT_VIEW (view),GTK_TEXT_BUFFER (buffer));
        }
        //check if there is a messege in the input update queue
        msgctl(upin_msg, IPC_STAT, &info_update_queue);
        if (info_update_queue.msg_qnum>0){
            msgrcv(upin_msg, upin_str, sizeof(input_m), 1, 0);
            #if DEBUG
                printf("thread update msg\n: %s\n",upin_str->mesg_text);
            #endif
            // display the message 
            printf("Data sent is \n: %s \n",  upin_str->mesg_text);
            GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
            gtk_text_buffer_get_end_iter (buffer,&iter);
            time_t t = time(NULL);
            char * cur_time = asctime(localtime(&t));
            gtk_text_buffer_insert(GTK_TEXT_BUFFER (buffer),&iter,"TU:\n",strlen("TU:\n"));
            gtk_text_buffer_insert(GTK_TEXT_BUFFER (buffer),&iter,upin_str->mesg_text,strlen(upin_str->mesg_text));
            gtk_text_buffer_insert(GTK_TEXT_BUFFER (buffer),&iter,cur_time,strlen(cur_time));
            gtk_text_view_set_buffer(GTK_TEXT_VIEW (view),GTK_TEXT_BUFFER (buffer));
        }
    }
    return NULL;
}
//function that handles the send button
static void callback( GtkWidget *widget,gpointer data )
{
    if (data==NULL){
        return;
    }
    GtkWidget* id = data;
    char* input = (char*)gtk_entry_get_text(GTK_ENTRY(id));
    if (strlen(input)<1){
        printf("NON PUOI MANDARE MESSAGGI VUOTI");
        memset(upin_str->mesg_text,0,sizeof(input_str->mesg_text));
        strcpy(upin_str->mesg_text,"NON PUOI MANDARE MESSAGGI VUOTI\n");
        msgsnd(upin_msg, upin_str, sizeof(input_m), 0);
    }
    else{
        memset(input_str->mesg_text,0,sizeof(input_str->mesg_text));
        memset(upin_str->mesg_text,0,sizeof(input_str->mesg_text));
        strcpy(input_str->mesg_text,input);
        strcpy(upin_str->mesg_text,input);
        strcat(upin_str->mesg_text,"\n");
        msgsnd(input_msg, input_str, sizeof(input_m),0);
        msgsnd(upin_msg, upin_str, sizeof(input_m), 0);
    }
    gtk_entry_set_text(GTK_ENTRY(id),"");
}
static void list_callback( GtkWidget *widget,gpointer data )
{
    if (data==NULL){
        return;
    }
    GtkWidget* id = data;
    char* input = "_LIST_";
    memset(input_str->mesg_text,0,sizeof(input_str->mesg_text));
    memset(upin_str->mesg_text,0,sizeof(upin_str->mesg_text));
    strcpy(input_str->mesg_text,input);
    strcpy(upin_str->mesg_text,input);
    strcat(upin_str->mesg_text,"\n");
    msgsnd(input_msg, input_str, sizeof(input_m),0);
    msgsnd(upin_msg, upin_str, sizeof(input_m), 0);
    gtk_entry_set_text(GTK_ENTRY(id),"");
}
//Gui that show after the login page
void main_page(GtkApplication *app,gpointer data){
    //AFTER LOGIN CODE
    int ret;
    pthread_t thread;
    ret = pthread_create(&thread, NULL,client,data);
    if (ret) handle_error_en(ret, "Could not create a new thread");
    
    if (DEBUG) fprintf(stderr, "New thread_client created to handle the request!\n");
    
    ret = pthread_detach(thread);
    if (ret) handle_error_en(ret, "Could not detach the thread");
    
    GtkWidget *window;
    GtkWidget *paned;
    GtkWidget *grid;
    GtkWidget *button;
    GtkWidget *list_button;
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
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    
    
    gtk_text_buffer_set_text(buffer,welcome_not_hacker,strlen(welcome_not_hacker));
    gtk_text_view_set_buffer(GTK_TEXT_VIEW (view),GTK_TEXT_BUFFER (buffer));



    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), 
                                    GTK_POLICY_AUTOMATIC, 
                                    GTK_POLICY_AUTOMATIC);

    gtk_container_add (GTK_CONTAINER (scrolled_window),view);

    paned=gtk_paned_new(GTK_ORIENTATION_VERTICAL);

    
    grid = gtk_grid_new ();

    /* Pack the container in the window */
   
    gtk_paned_add1 (GTK_PANED (paned),scrolled_window);
    gtk_paned_add2 (GTK_PANED (paned), grid);
    gtk_container_add (GTK_CONTAINER (window), paned);
    gtk_widget_set_size_request(paned,300,-1);
    



  
    button = gtk_button_new_with_label ("Send");
    list_button = gtk_button_new_with_label ("List");
    g_signal_connect (button, "clicked",G_CALLBACK (callback),im);
    g_signal_connect (list_button, "clicked",G_CALLBACK (list_callback),im);

    gtk_grid_attach(GTK_GRID(grid),im,0,0,4,1);
    gtk_grid_attach (GTK_GRID (grid), button, 0,5,2,1);
    gtk_grid_attach (GTK_GRID (grid), list_button, 3,5,2,1);


    gtk_widget_show_all (window);

    //Start of the thread that updates th gui
    // prepare arguments for the new thread
    handler_args_u* arg_up = (handler_args_u*)malloc(sizeof(handler_args_u));
    arg_up->view=view; 

    ret = pthread_create(&thread, NULL,update,arg_up);
    t_copy_updater = thread;
    if (ret) handle_error_en(ret, "Could not create a new thread");
    
    if (DEBUG) fprintf(stderr, "New thread_update created to handle the request!\n");
    
    ret = pthread_detach(thread); // I won't phtread_join() on this thread
    if (ret) handle_error_en(ret, "Could not detach the thread");
    
}
//Login page
void login( GtkWidget *widget,gpointer data ){
    struct user_par *arg = data; 
    GtkWidget* window = arg->window;
    GtkWidget* dialog; //= arg->dialog;
    GtkWidget* user = arg->username;
    GtkWidget* pas = arg->password;
    GtkApplication* app = arg->app;
    int socket_desc = arg->socket_desc;
    
    char* i_user = (char*)gtk_entry_get_text(GTK_ENTRY(user));
    char* i_pas = (char*)gtk_entry_get_text(GTK_ENTRY(pas));
    
    //sends his credential to server
    char snd[66];
    strcpy(snd,i_user);
    strcat(snd,";");
    strcat(snd,i_pas);
    strcat(snd,"\n");
    int usr_len = strlen(snd);
    printf("Usr len:%d, %s\n ",usr_len,snd);
    send_msg(socket_desc,snd,strlen(snd),0);

    #if DEBUG
        printf("Credential sent\n");
    #endif
    //wait for the response
    char ack[15];
    size_t ack_len = sizeof(ack);
    strcpy(ack,ERROR_MSG);
    memset(ack, 0, ack_len);
    
    recive_msg(socket_desc,ack,ack_len,0);
    printf("l'ack è: %s\n", ack);

    if (strcmp(ack,ERROR_MSG)){
        gtk_window_close(GTK_WINDOW(window));
        handler_args_m data_socket = {0};
        data_socket.socket_desc=socket_desc;
        main_page(app,&data_socket);
    }
    else{
        dialog = gtk_message_dialog_new(GTK_WINDOW (window),GTK_DIALOG_DESTROY_WITH_PARENT , GTK_MESSAGE_INFO,GTK_BUTTONS_NONE, "NOME UTENTE O PASSWORD SBAGLIATE" );
        gtk_dialog_run(GTK_DIALOG(dialog));

    }
    

}
//Function called by main to activate gui
static void activate (GtkApplication *app, gpointer data){

    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *button;
    GtkWidget *in_user;
    GtkWidget *in_pass;
    //GtkWidget *dialog;
    GtkWidget * usr_label;
    GtkWidget * pas_label;
    handler_args_m * data_socket = data;

    
    window = gtk_application_window_new (app);
    gtk_window_set_title (GTK_WINDOW (window), "Window");
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    gtk_window_set_default_size(GTK_WINDOW(window),100,100);

    in_user=gtk_entry_new ();
    in_pass=gtk_entry_new ();
    gtk_entry_set_visibility(GTK_ENTRY(in_pass),FALSE); 
    usr_label=gtk_label_new("Username");
    pas_label=gtk_label_new("Password");

    

    grid = gtk_grid_new ();

    button = gtk_button_new_with_label ("LOGIN");
    user_par.username=in_user;
    user_par.password=in_pass;
    user_par.window=window;
    user_par.app=app;
    user_par.socket_desc=data_socket->socket_desc;
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

    char buf[] = SERVER_COMMAND;

    int socket_desc=socket_desc_copy;
    
    // send message to server
    send_msg(socket_desc,buf,strlen(buf),0);
    pthread_cancel(t_copy_updater);
    pthread_cancel(t_copy_reciver);

    msgctl(update_msg, IPC_RMID, NULL);
    msgctl(input_msg, IPC_RMID, NULL);
    msgctl(upin_msg, IPC_RMID, NULL);
    free(input_str);
    free(upin_str);
    free(message);
    printf("quitting...\n");
    exit(EXIT_SUCCESS);

}
void handle_sigint(int sig) 
{ 
    printf("Caught signal %d\n", sig); 
    force_quit();
} 