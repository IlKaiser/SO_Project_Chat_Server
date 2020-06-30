#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <assert.h>

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
        if((ret==-1 || ret==0) && is_server){printf("Error send %d\n ",errno); return -1;}
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
    char buff[2048];
    do {                                
        ret = recv(socket_desc, buff + recv_bytes,1, 0);
        buff[recv_bytes]=='\0' ? printf("Got 0\n") : printf("Got char %c \n",buff[recv_bytes]);
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
    //printf("la chiave è: %s",key);
    //char prova[strlen((char*)key)];

    RSA *rsa= NULL;
    BIO *keybio ;
    keybio = BIO_new_mem_buf(key, -1);
    if (keybio==NULL)
    {
        printf( "Failed to create key BIO");
        return -1;
    }
    /*BIO_read(keybio,  prova , strlen((char*)key));
    printf("bio è: %s",prova);
    if (!strcmp(prova,(char*)key)) printf("SONO UGUALI\n");
    return -1;*/

    rsa = PEM_read_bio_RSAPublicKey(keybio, &rsa ,NULL, NULL);
    assert(rsa != NULL);
    //return -1;
    unsigned char encry[2100];
    int result = RSA_public_encrypt(data_len,data,encry,rsa,RSA_PKCS1_PADDING);
    if (result==-1){
        printf("encrypt error: %lu",ERR_get_error());
        return -1;
    }
    printf("Encrypted data size %d\n",result);
    printf("\n");
    printf("risultato %s with len %ld\n",(char*)encry,strlen((char*)encry));
    char b_encode[result+1];
    strcpy(b_encode,base64encode(encry,result));
    printf("after b_64 encrypt %s\n",b_encode);
    strcpy((char*)encrypted,b_encode);
    printf("risultato è after copy %s with len %ld\n",(char*)encrypted,strlen((char*)encrypted));
    return result;
}
int private_decrypt(unsigned char * enc_data,int data_len,unsigned char * key, unsigned char *decrypted)
{
    printf("la chiave è: %s",key);
    char prova[strlen((char*)key)];

    RSA *rsa= NULL;
    BIO *keybio ;
    keybio = BIO_new_mem_buf(key, -1);
    if (keybio==NULL)
    {
        printf( "Failed to create key BIO");
        return -1;
    }
    BIO_read(keybio,  prova , strlen((char*)key));
    printf("bio è: %s",prova);
    if (!strcmp(prova,(char*)key)) printf("SONO UGUALI\n");
    //return -1;

    unsigned char decry[2100];
    char b_decode[data_len+1];
    strcpy(b_decode,base64decode(enc_data,data_len));
    printf("after b_64 decrypt %s\n",b_decode);
    strcpy((char*)decry,b_decode);

    rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa ,NULL, NULL);
    if(rsa == NULL) {
        char errore[1024];
        printf("dencrypt error: %lu\n",ERR_get_error());
        ERR_error_string(ERR_get_error(),errore);
        printf("error: %s\n",errore);
        return -1;
    }
    //return -1;
    int  result = RSA_private_decrypt(data_len,enc_data,decry,rsa,RSA_PKCS1_PADDING);
    printf("risultato %s with len %ld\n",(char*)decry,strlen((char*)decry));
    strcpy((char*)decrypted,(char*)decry);
    printf("risultato è after copy %s with len %ld\n",(char*)decrypted,strlen((char*)decrypted));
    return result;
}

char *base64encode (const void *b64_encode_this, int encode_this_many_bytes){
    BIO *b64_bio, *mem_bio;      //Declares two OpenSSL BIOs: a base64 filter and a memory BIO.
    BUF_MEM *mem_bio_mem_ptr;    //Pointer to a "memory BIO" structure holding our base64 data.
    b64_bio = BIO_new(BIO_f_base64());                      //Initialize our base64 filter BIO.
    mem_bio = BIO_new(BIO_s_mem());                           //Initialize our memory sink BIO.
    BIO_push(b64_bio, mem_bio);            //Link the BIOs by creating a filter-sink BIO chain.
    BIO_set_flags(b64_bio, BIO_FLAGS_BASE64_NO_NL);  //No newlines every 64 characters or less.
    BIO_write(b64_bio, b64_encode_this, encode_this_many_bytes); //Records base64 encoded data.
    BIO_flush(b64_bio);   //Flush data.  Necessary for b64 encoding, because of pad characters.
    BIO_get_mem_ptr(mem_bio, &mem_bio_mem_ptr);  //Store address of mem_bio's memory structure.
    BIO_set_close(mem_bio, BIO_NOCLOSE);   //Permit access to mem_ptr after BIOs are destroyed.
    BIO_free_all(b64_bio);  //Destroys all BIOs in chain, starting with b64 (i.e. the 1st one).
    BUF_MEM_grow(mem_bio_mem_ptr, (*mem_bio_mem_ptr).length + 1);   //Makes space for end null.
    (*mem_bio_mem_ptr).data[(*mem_bio_mem_ptr).length] = '\0';  //Adds null-terminator to tail.
    return (*mem_bio_mem_ptr).data; //Returns base-64 encoded data. (See: "buf_mem_st" struct).
}

char *base64decode (const void *b64_decode_this, int decode_this_many_bytes){
    BIO *b64_bio, *mem_bio;      //Declares two OpenSSL BIOs: a base64 filter and a memory BIO.
    char *base64_decoded = calloc( (decode_this_many_bytes*3)/4+1, sizeof(char) ); //+1 = null.
    b64_bio = BIO_new(BIO_f_base64());                      //Initialize our base64 filter BIO.
    mem_bio = BIO_new(BIO_s_mem());                         //Initialize our memory source BIO.
    BIO_write(mem_bio, b64_decode_this, decode_this_many_bytes); //Base64 data saved in source.
    BIO_push(b64_bio, mem_bio);          //Link the BIOs by creating a filter-source BIO chain.
    BIO_set_flags(b64_bio, BIO_FLAGS_BASE64_NO_NL);          //Don't require trailing newlines.
    int decoded_byte_index = 0;   //Index where the next base64_decoded byte should be written.
    while ( 0 < BIO_read(b64_bio, base64_decoded+decoded_byte_index, 1) ){ //Read byte-by-byte.
        decoded_byte_index++; //Increment the index until read of BIO decoded data is complete.
    } //Once we're done reading decoded data, BIO_read returns -1 even though there's no error.
    BIO_free_all(b64_bio);  //Destroys all BIOs in chain, starting with b64 (i.e. the 1st one).
    return base64_decoded;        //Returns base-64 decoded data with trailing null terminator.
}
