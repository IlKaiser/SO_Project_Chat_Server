#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/socket.h>

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

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

    // remove trailing whitespace and \n
    while (ptr > src) {
        if (!isspace (*ptr) && *ptr!='\n')
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
int send_msg (int socket_desc,char* buf,int buf_size,char is_server){
    int bytes_sent=0;
    int ret;
    int msg_len = buf_size+1; //we always send the f***** \0
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
    } while (buff[recv_bytes++]!='\0');
    strncpy(buf,buff,buf_size);
    return 0;
}

void generatekeys(size_t pri_len,size_t pub_len,char *pri_key,char*pub_key){
    int ret;
    // Generate key pair
    printf("Generating RSA (%d bits) keypair...", 2048);
    fflush(stdout);
    BIGNUM* public_key_exponent = BN_new();
    BN_set_word(public_key_exponent, RSA_F4);
    RSA *keypair=RSA_new() ;
    ret = RSA_generate_key_ex(keypair,2048, public_key_exponent, NULL);

    if (!ret)  handle_error("Failed to create RSA context");

    BIO *pri = BIO_new(BIO_s_mem());
    BIO *pub = BIO_new(BIO_s_mem());

    PEM_write_bio_RSAPrivateKey(pri, keypair, NULL, NULL, 0, NULL, NULL);
    PEM_write_bio_RSAPublicKey(pub, keypair);

    pri_len = BIO_pending(pri);
    pub_len = BIO_pending(pub);

    //pri_key = malloc(pri_len + 1);
    //pub_key = malloc(pub_len + 1);

    size_t wrote_pri;
    size_t wrote_pub;

    BIO_read_ex(pri, pri_key, pri_len,&wrote_pri);
    BIO_read_ex(pub, pub_key, pub_len,&wrote_pub);



    pri_key[pri_len] = '\0';
    pub_key[pub_len] = '\0';

    printf("pub key len : %ld\n",strlen(pub_key));
    printf("pri key len : %ld\n",strlen(pri_key));

    printf("pub key wrote : %ld\n",wrote_pub);
    printf("pri key wrote : %ld\n",wrote_pri);

    #ifdef DEBUG
        printf("\n%s\n%s\n", pri_key, pub_key);
    #endif
    printf("done.\n");
}

int public_encrypt(unsigned char * data,int data_len,unsigned char * key, unsigned char *encrypted)
{
    BIO *keybio ;
    keybio = BIO_new_mem_buf(key, -1);
    RSA * rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa,NULL, NULL);
    printf( "la size della rsa è: %d\n",RSA_size(rsa));
    int result = RSA_public_encrypt(data_len,data,encrypted,rsa,RSA_PKCS1_PADDING);
    return result;
}
int private_decrypt(unsigned char * enc_data,int data_len,unsigned char * key, unsigned char *decrypted)
{
    BIO *keybio ;
    keybio = BIO_new_mem_buf(key, -1);
    RSA * rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa,NULL, NULL);
    int  result = RSA_private_decrypt(data_len,enc_data,decrypted,rsa,RSA_PKCS1_PADDING);
    return result;
}