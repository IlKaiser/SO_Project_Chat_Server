#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>

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
void send_msg (int socket_desc,char* buf){
    int bytes_sent=0;
    int ret;
    int msg_len = strlen(buf);
    while ( bytes_sent < msg_len) {
        ret = send(socket_desc, buf + bytes_sent, msg_len - bytes_sent, 0);
        if (ret == -1 && errno == EINTR) continue;
        if (ret == -1) handle_error("Cannot write to the socket");
        bytes_sent += ret;
    }
}
void recive_msg(int socket_desc,char* buf){
    int recv_bytes=0;
    int ret;
    memset(buf,0,sizeof(buf));
    do {                                
        ret = recv(socket_desc, buf + recv_bytes,1, 0);
        if (ret == -1 && errno == EINTR) continue;
        if (ret == -1) handle_error("Cannot read from the socket");
        if (ret == 0) handle_error_en(0xDEAD,"server is offline");
    } while (buf[recv_bytes++]!='\0');
}