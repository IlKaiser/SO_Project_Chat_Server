#include<string.h>
#include<stdio.h>


#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#include "Common/common.h"
int main(){
    const unsigned char* enc="Stringa **********";
    char benc[100];
    char bdec[100];
    strcpy(benc,base64encode(enc,strlen(enc)));
    strcpy(bdec,base64decode(benc,strlen(benc)));
    printf("Decoded string %s\n",bdec);




    int ret;
    // Generate key pair
    printf("Generating RSA (%d bits) keypair...\n", 2048);
    BIGNUM* public_key_exponent = BN_new();
    BN_set_word(public_key_exponent, RSA_F4);
    RSA *keypair=RSA_new() ;
    ret = RSA_generate_key_ex(keypair,2048, public_key_exponent, NULL);

    if (!ret)  handle_error("Failed to create RSA context");

    BIO *pri = BIO_new(BIO_s_mem());
    BIO *pub = BIO_new(BIO_s_mem());

    PEM_write_bio_RSAPrivateKey(pri, keypair, NULL, NULL, 0, NULL, NULL);
    PEM_write_bio_RSAPublicKey(pub, keypair);

    int pri_len = BIO_pending(pri);
    int pub_len = BIO_pending(pub);

    //pri_key = malloc(pri_len + 1);
    //pub_key = malloc(pub_len + 1);

    char pri_key[pri_len+1];
    char pub_key[pub_len+1];

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

    RSA *rsa= NULL;
    BIO *keybio ;
    keybio = BIO_new_mem_buf(pub_key, -1);
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
    //return -1;
    unsigned char encry[2100];
    unsigned char encrypted[2100];
    int result = RSA_public_encrypt(strlen((char*)enc),enc,encry,rsa,RSA_PKCS1_PADDING);
    if (result==-1){
        printf("encrypt error: %lu",ERR_get_error());
        return -1;
    }
    printf("Encrypted data size %d\n",result);
    printf("\n");
    printf("risultato %s with len %ld\n",(char*)encry,strlen((char*)encry));
    /*char b_encode[result+1];
    strcpy(b_encode,base64encode(encry,result));
    printf("after b_64 encrypt %s\n",b_encode);
    strcpy((char*)encrypted,b_encode);*/


    printf("risultato è after copy %s with len %ld\n",(char*)encrypted,strlen((char*)encrypted));
    printf("Data %s with len %ld\n",encrypted,sizeof(encrypted));
    
    /*printf("la chiave è: %s",key);
    char prova[strlen((char*)key)];*/

    rsa= NULL;
    keybio = BIO_new_mem_buf(pri_key, -1);
    if (keybio==NULL)
    {
        printf( "Failed to create key BIO");
        return -1;
    }
    /*BIO_read(keybio,  prova , strlen((char*)key));
    printf("bio è: %s",prova);
    if (!strcmp(prova,(char*)key)) printf("SONO UGUALI\n");
    //return -1;*/

    unsigned char decry[2100];
    /*char b_decode[256+1];*/
    unsigned char decrypted[2100];
    /*strcpy(b_decode,base64decode(encrypted,256));
    printf("after b_64 decrypt %s\n",b_decode);*/
    //strcpy((char*)decry,b_decode);

    rsa = PEM_read_bio_RSAPrivateKey(keybio, NULL ,NULL, NULL);
    if(rsa == NULL) {
        char errore[1024];
        printf("dencrypt error: %lu\n",ERR_get_error());
        ERR_error_string(ERR_get_error(),errore);
        printf("error: %s\n",errore);
        return -1;
    }
    //return -1;
    result = RSA_private_decrypt(256,(unsigned char*)encry,decry,rsa,RSA_PKCS1_PADDING);
    if(result == -1){
        char err[1024];
        ERR_error_string(ERR_get_error(),err);
        printf("Errore decry%s\n",err);
    }
    printf("Decoded ints %d\n",result);
    printf("decry risultato %s with len %ld\n",(char*)decry,strlen((char*)decry));
    strcpy((char*)decrypted,(char*)decry);
    printf("decry risultato è after copy %s with len %ld\n",(char*)decrypted,strlen((char*)decrypted));
    return result;
}