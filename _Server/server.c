#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <arpa/inet.h>  // htons()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */

#include <libpq-fe.h>



#include "server.h"



/* Memory shared between all threads */

//Connection Arrays
char* user_names[MAX_SIZE];
int sockets[MAX_SIZE];


//Previous size of connection arrays
int previous_size=0;
//Current size of connection arrays
int current_size=0;
// Next position in arrays
int next_position=0;
//Semaphore for mutual exclusion
sem_t* sem;


int main(int argc, char* argv[]) {
    // ignore sigpipe so we can handle disconnection errrors manually
    signal(SIGPIPE,SIG_IGN);

    int ret;

    int socket_desc;

    //init semaphore
    sem_unlink(SEM_NAME);
    sem=sem_open(SEM_NAME,O_CREAT|O_EXCL,0644,1);
    if(sem==SEM_FAILED){
        handle_error("Error creating semaphore");
    }    

    
    // some fields are required to be filled with 0
    struct sockaddr_in server_addr = {0};

     // we will reuse it for accept()

    // initialize socket for listening
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc < 0) handle_error("Could not create socket");

    server_addr.sin_addr.s_addr = INADDR_ANY; // we want to accept connections from any interface
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(SERVER_PORT); // don't forget about network byte order!
    //inet_aton(SERVER_ADDRESS, &server_addr.sin_addr);

    /* We enable SO_REUSEADDR to quickly restart our server after a crash:
     * for more details, read about the TIME_WAIT state in the TCP protocol */
    int reuseaddr_opt = 1;
    ret = setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
    if (ret) handle_error("Cannot set SO_REUSEADDR option");

    // bind address to socket
    ret = bind(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
    if (ret) handle_error("Cannot bind address to socket");

    // start listening
    ret = listen(socket_desc, MAX_CONN_QUEUE);
    if (ret) handle_error("Cannot listen on socket");

    printf("%s","Server Online\n");
    fflush(stdout);

    mthreadServer(socket_desc);


    exit(EXIT_SUCCESS); // this will never be executed
}

void connection_handler(int socket_desc, struct sockaddr_in* client_addr) {


    int error = 0;
    socklen_t len = sizeof (error);
    
    int ret, recv_bytes,bytes_sent;

    char buf[1024];
    size_t buf_len = sizeof(buf);
    int msg_len;

    char* quit_command = SERVER_COMMAND;
    char* list_command = LIST_COMMAND;
    char credentials[66];
    size_t quit_command_len = strlen(quit_command);
    size_t list_command_len = strlen(list_command);

    #if DEBUG
        //Print socket desc number for double checking
        printf("Got socket desc %d\n",socket_desc);
    #endif

    // parse client IP address and port
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr->sin_addr), client_ip, INET_ADDRSTRLEN);
    uint16_t client_port = ntohs(client_addr->sin_port); // port number is an unsigned short

    #if DEBUG
        printf("Client ip %s, client port %hu\n",client_ip,client_port);
    #endif // DEBUG


    char* user_name=(char*)malloc(33*sizeof(char));
    user_name=ERROR_MSG;
    char* tok=(char*)malloc(33*sizeof(char));

    // 1. get client username and password finchè non arrivano quelle corrette
    while(!strcmp(user_name,ERROR_MSG)){
        memset(credentials, 0, strlen(credentials));
        recv_bytes = 0;
        do {
            ret = recv(socket_desc, credentials + recv_bytes, 1, 0);
            if (ret == -1 && errno == EINTR) continue;
            // Of course i still love you
            if (ret == -1 || ret==0) disconnection_handler(socket_desc);
        } while ( credentials[recv_bytes++] != '\n' );

        printf("sono qui\n");
        fflush(stdout);
        printf("le credenziali sono %s \n",credentials );
        if (login(credentials,socket_desc)){
            printf("copio username \n");
            tok = strtok(credentials, ";");
            printf("%s\n",tok);

        }
        else{
            printf("copio errore \n");
            strcpy(tok,ERROR_MSG);

        }
        printf("il risultato è %s\n",tok);
        fflush(stdout);

        if(!strcmp(tok,ERROR_MSG)){
            printf("INVALID CREDENTIALS\n");
            fflush(stdout);
            memset(buf, 0, buf_len);
            strcpy(buf,ERROR_MSG);
            bytes_sent = 0;
            msg_len = strlen(buf);
            while ( bytes_sent < msg_len){
                ret = send(socket_desc, buf + bytes_sent, msg_len - bytes_sent, 0);
                if (ret == -1 && errno == EINTR) continue;
                // Of course i still love you
                if (ret == -1) disconnection_handler(socket_desc);
                bytes_sent += ret;
            }
        }
        user_name=(char*)malloc(33*sizeof(char));
        strcpy(user_name,tok);
        strcat(user_name,"\n");
    }

    #if DEBUG
        printf("Username get: %s\n",user_name);
    #endif // DEBUG

    // 1.1 Updates global variables info
    if(previous_size==MAX_SIZE || (strcmp(user_name,"")==0)){
        memset(buf, 0, buf_len);
        strcpy(buf,ERROR_MSG);
        bytes_sent = 0;
        msg_len = strlen(buf);
	    while ( bytes_sent < msg_len){
            ret = send(socket_desc, buf + bytes_sent, msg_len - bytes_sent, 0);
            if (ret == -1 && errno == EINTR) continue;
            // Of course i still love you
            if (ret == -1) disconnection_handler(socket_desc);
            bytes_sent += ret;
        }
        disconnection_handler(socket_desc);
    }else{
        //Critical section shared mem writing
        //updates shared array
        ret=sem_wait(sem);
        if(ret){
            handle_error("Err sem wait");
        }
        previous_size=current_size;
        user_names[next_position]=user_name;
        sockets[next_position]=socket_desc;
        current_size++;
        // determinate next free position
        set_next_position();


        ret=sem_post(sem);
        if(ret){
            handle_error("Err sem post");
        }
        // 1.2 send first ack to client
        memset(buf, 0, buf_len);
        strcpy(buf,OK_MSG);
        bytes_sent = 0;
        msg_len = strlen(buf);
	    while ( bytes_sent < msg_len){
            ret = send(socket_desc, buf + bytes_sent, msg_len - bytes_sent, 0);
            if (ret == -1 && errno == EINTR) continue;
            // Of course i still love you
            if (ret == -1) disconnection_handler(socket_desc);
            bytes_sent += ret;
        }
    }



    // 1.2 Open db connection
    const char *conninfo = "hostaddr=15.236.174.17 port=5432 dbname=postgres user=postgres password=Quindicimaggio_20 sslmode=disable";
    PGconn *conn;
    PGresult *res;
    

    conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    }

    // 2. check if there is only one client
    LOOP:while(current_size<2){

        //test if the client is still up
        int retval = getsockopt (socket_desc, SOL_SOCKET, SO_ERROR, &error, &len);
        if(retval){

            disconnection_handler(socket_desc);
        }
        memset(buf, 0, buf_len);
        strcpy(buf,ALONE_MSG);
        #if DEBUG
            printf("Alone\n");
        #endif // DEBUG
        msg_len = strlen(buf)+1;//we include \0
         #if DEBUG
            printf("Buf: %s\n",buf);
        #endif // DEBUG
        bytes_sent = 0;
	    while ( bytes_sent < msg_len) {
            ret = send(socket_desc, buf + bytes_sent, msg_len - bytes_sent, 0);
            if (ret == -1 && errno == EINTR) continue;
            if (ret == -1) disconnection_handler(socket_desc);
            bytes_sent += ret;
        }
        ret=sleep(5);
        if(ret){
            handle_error("Err sleep");
        }
    }
    // 3. send user list
    list_formatter(buf,socket_desc);
    #if DEBUG
        printf("List %s",buf);
    #endif
    msg_len = strlen(buf)+1; //we include \0
    bytes_sent = 0;
	while ( bytes_sent < msg_len) {
        ret = send(socket_desc, buf + bytes_sent, msg_len - bytes_sent, 0);
        if (ret == -1 && errno == EINTR) continue;
        // Of course i still love you
        if (ret == -1) disconnection_handler(socket_desc);
        bytes_sent += ret;
    }
    // 4. get id number from client
    char user_buf[4];
    recv_bytes=0;
    do {
        ret = recv(socket_desc, user_buf + recv_bytes, 1, 0);
        if (ret == -1 && errno == EINTR) continue;
        // Of course i still love you
        if (ret == -1 || ret==0) disconnection_handler(socket_desc);
        //check if we are about to overflow the buffer
        if(recv_bytes>3){
            printf("Recived almost 4 bytes, resetting buffer...\n");
            memset(user_buf,0,4);
            recv_bytes=0;
        }
	}while ( user_buf[recv_bytes++]!= '\n' );
    printf("Buffer %s \n",user_buf);
    int user_id=atoi(user_buf);
    #if DEBUG
        printf("User id chosen: %d\n",user_id);
    #endif // DEBUG

    int socket_target=DISCONNECTED;
    char* target_user_name=NULL;
    // 4.1 get target socket desc
    if(user_id<=current_size){
         socket_target=sockets[user_id-1];

        //Look for target user name
        target_user_name=user_names[user_id-1];
    }
    
    //check if user his number
    if (socket_target==socket_desc){
        memset(buf, 0, buf_len);
        strcpy(buf,ERROR_MSG);
        bytes_sent = 0;
        msg_len = strlen(buf);
        while ( bytes_sent < msg_len){
            ret = send(socket_desc, buf + bytes_sent, msg_len - bytes_sent, 0);
            if (ret == -1 && errno == EINTR) continue;
            // Of course i still love you
            if (ret == -1) disconnection_handler(socket_desc);
            bytes_sent += ret;
        }
        disconnection_handler(socket_desc);
    }

    if(socket_target != DISCONNECTED){
        #if DEBUG
            printf("socket target number : %d\n",socket_target);
        #endif // DEBUG

        //4.2 send second ack //replacing the second ack with old messages
        // query for old messages
        //connetto al db
        const char *conninfo = "hostaddr=15.236.174.17 port=5432 dbname=postgres user=postgres password=Quindicimaggio_20 sslmode=disable";
        PGconn *conn;
        PGresult *res;
        

        conn = PQconnectdb(conninfo);
        if (PQstatus(conn) != CONNECTION_OK)
        {
            fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
            PQfinish(conn);
            exit(1);
        }
        printf ("searching messages between %s %s",user_name,target_user_name);
        const char* paramValue[2] = {user_name,target_user_name};
        res = PQexecParams(conn,"select mess._fro,mess.co,to_char(mess.data, 'DD-MM-YYYY HH24:MI') from( select m._from as _fro, m.mes as co, m.data as data from messaggi as m where m._from=$1 and m._to=$2 union all select m1._from as _fro, m1.mes as co, m1.data as data from messaggi as m1 where m1._from=$2 and m1._to=$1) as mess order by mess.data desc limit 5",
                        2,       /* two param*/
                        NULL,    /* let the backend deduce param type*/
                        paramValue,
                        NULL,
                        NULL,
                        1);      /* ask for binary results*/
        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
            PQclear(res);
            exit_nicely(conn,res,socket_desc);
        }
        int rows=PQntuples(res);
        if (rows==0){
            bytes_sent = 0;
            strcpy(buf,OK_MSG);
            msg_len = strlen(buf);
            #if DEBUG
                printf("Sending 2nd ack %s with len %d \n",buf,msg_len);
            #endif
            while ( bytes_sent < msg_len){
                ret = send(socket_desc, buf + bytes_sent, msg_len - bytes_sent, 0);
                if (ret == -1 && errno == EINTR) continue;
                // Of course i still love you
                if (ret == -1) disconnection_handler(socket_desc);
                bytes_sent += ret;
                printf("bytes sent %d\n",bytes_sent);
            }
        }
        else{
            bytes_sent = 0;
            strcpy(buf,MSG_MSG);
            msg_len = strlen(buf);
            #if DEBUG
                printf("Sending 2nd ack %s with len %d \n",buf,msg_len);
            #endif
            while ( bytes_sent < msg_len){
                ret = send(socket_desc, buf + bytes_sent, msg_len - bytes_sent, 0);
                if (ret == -1 && errno == EINTR) continue;
                // Of course i still love you
                if (ret == -1) disconnection_handler(socket_desc);
                bytes_sent += ret;
                printf("bytes sent %d\n",bytes_sent);
            }
            memset(buf, 0, buf_len);
            int ro;
            int co;

            for (ro=0;ro<rows;ro++){
                for (co=0;co<4;co++){
                    strcat(buf,PQgetvalue(res,ro,co));
                }
                strcat(buf,";");
            }
            strcat(buf,"\n");
            bytes_sent = 0;
            msg_len = strlen(buf);
            #if DEBUG
                printf("Sending old messages %s with len %d \n",buf,msg_len);
            #endif
            while ( bytes_sent < msg_len){
                ret = send(socket_desc, buf + bytes_sent, msg_len - bytes_sent, 0);
                if (ret == -1 && errno == EINTR) continue;
                // Of course i still love you
                if (ret == -1) disconnection_handler(socket_desc);
                bytes_sent += ret;
                printf("bytes sent %d\n",bytes_sent);
            }
        }

    }else{
        memset(buf, 0, buf_len);
        strcpy(buf,ERROR_MSG);
        bytes_sent = 0;
        msg_len = strlen(buf);
        while ( bytes_sent < msg_len){
            ret = send(socket_desc, buf + bytes_sent, msg_len - bytes_sent, 0);
            if (ret == -1 && errno == EINTR) continue;
            // Of course i still love you
            if (ret == -1) disconnection_handler(socket_desc);

            bytes_sent += ret;
        }
        disconnection_handler(socket_desc);
    }

    #if DEBUG
        printf("Send second ack %s\n",buf);
    #endif // DEBUG
    // reciver loop 
    while (1) {

        //5. main loop

        // read message from client
        #if DEBUG
            printf("Enter main loop \n");
        #endif // DEBUG
        memset(buf,0,buf_len);
        recv_bytes = 0;
        do {
            ret = recv(socket_desc, buf + recv_bytes, 1, 0);
            if (ret == -1 && errno == EINTR) continue;
            // Of course i still love you
            if (ret == -1 || ret == 0) disconnection_handler(socket_desc);
            if(recv_bytes>1022){
            printf("Recived almost 1024 bytes, resetting buffer...\n");
            memset(buf,0,buf_len);
            recv_bytes=0;
        }
		} while ( buf[recv_bytes++] != '\n' );
        #if DEBUG
            printf("Message %s from %s",buf,user_name);
        #endif // DEBUG

        // check whether I have just been told to quit...
        if (recv_bytes == 0) disconnection_handler(socket_desc);
        if (recv_bytes == quit_command_len && !memcmp(buf, quit_command, quit_command_len)){ 
            printf("Quitting...\n");
            disconnection_handler(socket_desc);
        }
        if (recv_bytes == list_command_len && !memcmp(buf,list_command,list_command_len)){
            printf("Send new list\n");
            goto LOOP;
        }
        // ... or if I have to send the message back
        // 5.1 insert msg into db
        time_t t = time(NULL);
        char * cur_time = asctime(localtime(&t));
        printf("local:     %s", cur_time);
        const char* paramValue[4] = {user_name,target_user_name,buf,cur_time};
        res = PQexecParams(conn,
                       "INSERT INTO messaggi (_from,_to,mes,data) VALUES ($1,$2,$3,$4)",
                       4,       /* one param */
                       NULL,    /* let the backend deduce param type */
                       paramValue,
                       NULL,
                       NULL,
                       1);      /* ask for binary results */
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            fprintf(stderr, "INSERT failed: %s", PQerrorMessage(conn));
            exit_nicely(conn,res,socket_desc);
        }
        PQclear(res);



        //5.2 Send to requested target
        char to_send[1024];
        memset(to_send,0,sizeof(to_send));
        bytes_sent = 0;
        strcat(buf,"\n");
        strcat(to_send,user_name);
        strcat(to_send,buf);
        msg_len = strlen(to_send)+1;
        while ( bytes_sent < msg_len){
            ret = send(socket_target, to_send + bytes_sent, msg_len - bytes_sent, 0);
            if (ret == -1 && errno == EINTR) continue;
            // Of course i still love you
            if (ret == -1) disconnection_handler(socket_desc);
            bytes_sent += ret;
        }
        #if DEBUG
            printf("sto mandando: %s",to_send);
        #endif
    } 
    //If a break occurs
    disconnection_handler(socket_desc);
}

// Wrapper function that take as input handler_args_t struct and then call 
// connection_handler.
void *thread_connection_handler(void *arg) {
    handler_args_t *args = (handler_args_t *)arg;
    int socket_desc = args->socket_desc;
    struct sockaddr_in *client_addr = args->client_addr;
    connection_handler(socket_desc,client_addr);
    // do not forget to free client_addr! by design it belongs to the
    // thread spawned for handling the incoming connection
    free(args->client_addr);
    free(args);
    pthread_exit(NULL);
}

void mthreadServer(int server_desc) {
    int ret = 0;

    // loop to manage incoming connections spawning handler threads
    int sockaddr_len = sizeof(struct sockaddr_in);
    while (1) {
        // we dynamically allocate a fresh client_addr object at each
        // loop iteration and we initialize it to zero
        struct sockaddr_in* client_addr = calloc(1, sizeof(struct sockaddr_in));
        
        // accept incoming connection
        int client_desc = accept(server_desc, (struct sockaddr *) client_addr, (socklen_t *)&sockaddr_len);
        
        if (client_desc == -1 && errno == EINTR) continue; // check for interruption by signals
        if (client_desc < 0) handle_error("Cannot open socket for incoming connection");
        
        if (DEBUG) fprintf(stderr, "Incoming connection accepted...\n");

        pthread_t thread;

        // prepare arguments for the new thread
        handler_args_t *thread_args = malloc(sizeof(handler_args_t));
        thread_args->socket_desc = client_desc;
        thread_args->client_addr = client_addr;
        ret = pthread_create(&thread, NULL, thread_connection_handler, (void *)thread_args);
        if (ret) handle_error_en(ret, "Could not create a new thread");
        
        if (DEBUG) fprintf(stderr, "New thread created to handle the request!\n");
        
        ret = pthread_detach(thread); // I won't phtread_join() on this thread
        if (ret) handle_error_en(ret, "Could not detach the thread");
    }
}

void list_formatter(char buf[],int socket_desc){
    memset(buf, 0,strlen(buf));
    int i;
    for (i=0;i<current_size;i++){ 
        if(sockets[i]!=DISCONNECTED){ 
            char number[15];
            sprintf(number, "%d: ",i+1);
            strcat(buf,number);
            if(sockets[i]==socket_desc){
                strcat(buf,"[YOU]\n");
            }else{
                strcat(buf,user_names[i]);
            }
            strcat(buf,"\n");
        }
    }
}

static void exit_nicely(PGconn *conn, PGresult   *res,int socket_desc){
    PQclear(res);
    PQfinish(conn);
    disconnection_handler(socket_desc);
}

void disconnection_handler(int index){
    int ret;
    #if DEBUG
        printf("Index got in handler %d\n",index);
    #endif
    if(index!=-1){
        ret=close(index);
        if(ret){handle_error("Disconnection error");}
        set_disconnected(index);
        set_next_position();
    }
    ret=sem_wait(sem);
    current_size--;
    ret|=sem_post(sem);
    if(ret){handle_error("Disconnection semaphore error");}
    pthread_exit(NULL);
}

void set_next_position(){
    int i;
    for(i=0;i<current_size;i++){
        if(sockets[i]==DISCONNECTED){
            next_position=i;
            return;
        }
    }
    next_position=current_size;
    return;
}

void set_disconnected(int socket_desc){
    int i;
    for(i=0;i<current_size;i++){
        if(sockets[i]==socket_desc){
            sockets[i]=DISCONNECTED;
            return;
        }
    }
}

int login(char* credentials,int socket_desc){
    //int read_bytes = 0;
    char username[32];
    char password[32];
    char * token = strtok(credentials, ";");
    strcpy(username,token ); //salvo username
    token = strtok(NULL, ";");
    strcpy(password,token);//salvo password
    password[strlen(password)-1]='\0';
    //connetto al db
    const char *conninfo = "hostaddr=15.236.174.17 port=5432 dbname=postgres user=postgres password=Quindicimaggio_20 sslmode=disable";
    PGconn *conn;
    PGresult *res;
    

    conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    }
    printf("hai inserito: %s,%s \n",username,password);
    const char* paramValue[2] = {username,password};
    res = PQexecParams(conn,
                    "select username from users where username=$1 and password=$2",
                    2,       /* two param*/
                    NULL,    /* let the backend deduce param type*/
                    paramValue,
                    NULL,
                    NULL,
                    1);      /* ask for binary results*/
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        exit_nicely(conn,res,socket_desc);
    }
    int ris=PQntuples(res);
    return ris;
}