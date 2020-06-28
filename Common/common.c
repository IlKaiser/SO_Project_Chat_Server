#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "common.h"


void trim (char *dest, char *src){
    if (!src || !dest)
       return;
    int len = strlen (src);
    if (!len) {
        *dest = '\0';
        return;
    }
    char *ptr = src + len - 1;

    // remove trailing whitespace
    while (ptr > src) {
        if (!isspace (*ptr))
            break;
        ptr--;
    }
    ptr++;
    char *q;
    // remove leading whitespace
    for (q = src; (q < ptr && isspace (*q)); q++)
        ;
    while (q < ptr)
        *dest++ = *q++;
    *dest = '\0';
}
int send_msg (int socket_desc,char* buf,char is_server){
    int bytes_sent=0;
    int ret;
    int msg_len = strlen(buf)+1; //we always send the f***** \0
    while ( bytes_sent < msg_len) {
        ret = send(socket_desc, buf + bytes_sent, msg_len - bytes_sent, 0);
        if (ret == -1 && errno == EINTR) continue;
        if((ret==-1 || ret==0) && is_server) return -1;
        else{
            /// Called from client
            if (ret == -1) handle_error("Cannot read from the socket");
            if (ret == 0) handle_error_en(0xDEAD,"server is offline");
        }
        bytes_sent += ret;
    }
    return 0;
}
int recive_msg(int socket_desc,char* buf,int buf_size,char is_server){
    int recv_bytes=0;
    int ret;
    char buff[1024];
    do {                                
        ret = recv(socket_desc, buff + recv_bytes,1, 0);
        if (ret == -1 && errno == EINTR) continue;
        /// Of course we still love you (if called from server)
        if((ret==-1 || ret==0) && is_server) return -1;
        else{
            /// Called from client
            if (ret == -1) handle_error("Cannot read from the socket");
            if (ret == 0) handle_error_en(0xDEAD,"server is offline");
        }
        /// Overflow check
        if(recv_bytes>buf_size-2){
            printf("Recived almost %d bytes, resetting buffer...\n",buf_size);
            memset(buff,0,buf_size);
            recv_bytes=0;
        }
    } while (buf[recv_bytes++]!='\0');
    strncpy(buf,buff,strlen(buff));
    return 0;
}