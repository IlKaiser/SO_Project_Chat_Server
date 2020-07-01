#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/aes.h>

#define N_KEYS      3
#define N_BITS      3072


EVP_PKEY** generate_rsa_keys(int count, int bits)
{
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    EVP_PKEY **keys = OPENSSL_malloc(count * sizeof *keys);
    for (int i = 0; i < count; ++i)
    {
        // initialize generator
         EVP_PKEY_keygen_init(ctx);
         EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, bits);

        // generate the key pair
        keys[i] = NULL;
         EVP_PKEY_keygen(ctx, keys + i);
    }
    EVP_PKEY_CTX_free(ctx);

    return keys;
}

int main()
{
    OpenSSL_add_all_algorithms();
    OpenSSL_add_all_ciphers();

    // used throughout the code below
    BIO *bio = BIO_new_fp(stdout, BIO_NOCLOSE);

    // simple message to encrypt.
    char message[] = "The quick brown fox jumps over the lazy dog.";
    size_t message_length = strlen(message);

    // algorithm aes256-cbc
    const EVP_CIPHER *cipher = EVP_aes_256_cbc();
    uint8_t iv[AES_BLOCK_SIZE];

    // generate all rsa key pairs
    EVP_PKEY **keys = generate_rsa_keys(N_KEYS, N_BITS);

    // create encryption targets and public-only keys. note the latter here
    //  isn't really necessary, since we could just use the keys[] entries,
    //  but we're simulating what happens when you only have the public keys
    //  for sealing.
    EVP_PKEY **pubkeys = OPENSSL_malloc(N_KEYS * sizeof *pubkeys);
    uint8_t **enc_key = OPENSSL_malloc(N_KEYS * sizeof(*enc_key));
    int *enc_key_length = OPENSSL_malloc(N_KEYS * sizeof *enc_key_length);
    for (int i = 0; i < N_KEYS; ++i)
    {
        RSA* rsa = EVP_PKEY_get1_RSA(keys[i]);
        enc_key[i] = OPENSSL_malloc(RSA_size(rsa));
        pubkeys[i] = EVP_PKEY_new();
        EVP_PKEY_assign_RSA(pubkeys[i], RSAPublicKey_dup(rsa));
        RSA_free(rsa);
    }

    ////////////////////////////////////////////////////////////////////////////
    // SEAL AND GENERATE SESSION KEY ENCRYPTIONS
    //int tmp, encrypted_length = AES_BLOCK_SIZE * ((message_length + AES_BLOCK_SIZE) / AES_BLOCK_SIZE);
    //uint8_t *encrypted = OPENSSL_malloc(encrypted_length);
    EVP_CIPHER_CTX *cctx = EVP_CIPHER_CTX_new();
    EVP_SealInit(cctx, cipher, enc_key, enc_key_length, iv, pubkeys,1);
    int i = 0 ;
    char prova[] = "peppino";
     int encrypted_length = AES_BLOCK_SIZE * ((strlen(prova) + AES_BLOCK_SIZE) / AES_BLOCK_SIZE);
    while (i<3)
    {   
        printf("prova %s\n",prova);
        uint8_t *encrypted = OPENSSL_malloc(encrypted_length);
        char *decrypted = OPENSSL_malloc(encrypted_length);
        int outl = 0,tmp1,tmp2;
        EVP_SealUpdate(cctx,(unsigned char*) encrypted, &encrypted_length,(unsigned char*) prova, strlen(prova));
        EVP_SealFinal(cctx, encrypted + encrypted_length, &tmp1);
        encrypted_length += tmp1;

        BIO_puts(bio, "Encrypted Message\n");
        BIO_dump(bio, (const char*)encrypted, encrypted_length);

        strlen((char*)encrypted) == encrypted_length ? printf("OK\n") : printf("Strlen %d, length %d\n",strlen((char*)encrypted),encrypted_length); 

        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        EVP_OpenInit(ctx, cipher, enc_key[0], enc_key_length[0], iv, keys[0]);
        EVP_OpenUpdate(ctx,(unsigned char*) decrypted, &outl, encrypted, strlen(encrypted));
        EVP_OpenFinal(ctx,(unsigned char*) decrypted + outl, &tmp2);
        outl += tmp2;
        BIO_printf(bio, "Decrypted Message(%d)\n", i + 1);
        BIO_dump(bio, (const char*)decrypted, outl);
        i++;
        sprintf(prova,"%d",i);
        EVP_CIPHER_CTX_cleanup(ctx);
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_free(decrypted);
        OPENSSL_free(encrypted);
    }
    return 0;
    //EVP_SealFinal(cctx, encrypted + encrypted_length, &tmp);
   /* encrypted_length += tmp;
    EVP_CIPHER_CTX_free(cctx);


    ////////////////////////////////////////////////////////////////////////////
    // DUMP ENCRYPTED KEYS & IV
    for (int i = 0; i < N_KEYS; ++i)
    {
        BIO_printf(bio, "Encrypted Session Key(%d)\n", i + 1);
        BIO_dump(bio, (const char*)enc_key[i], enc_key_length[i]);
    }
    BIO_puts(bio, "IV\n");
    BIO_dump(bio, (const char*)iv, AES_BLOCK_SIZE);


    ////////////////////////////////////////////////////////////////////////////
    // DUMP ENCRYYPTED MESSAGE
    BIO_puts(bio, "Encrypted Message\n");
    BIO_dump(bio, (const char*)encrypted, encrypted_length);


    ////////////////////////////////////////////////////////////////////////////
    // TEST EACH KEY DECRYPTION
    for (int i = 0; i < N_KEYS; ++i)
    {
        char *decrypted = OPENSSL_malloc(encrypted_length);
        int outl = 0, tmp;

        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        EVP_OpenInit(ctx, cipher, enc_key[i], enc_key_length[i], iv, keys[i]);
        EVP_OpenUpdate(ctx, decrypted, &outl, encrypted, encrypted_length);
        EVP_OpenFinal(ctx, decrypted + outl, &tmp);
        outl += tmp;

        BIO_printf(bio, "Decrypted Message(%d)\n", i + 1);
        BIO_dump(bio, (const char*)decrypted, outl);

        // cleanup the context and dynamic allocations
        EVP_CIPHER_CTX_cleanup(ctx);
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_free(decrypted);
    }

    ////////////////////////////////////////////////////////////////////////////
    // TEST SESSION KEY DECRYPTION USING PRIVATE KEYS
    //  (these should all display the same session key)
    for (int i = 0; i < N_KEYS; ++i)
    {
        uint8_t *out = NULL;
        size_t outl = 0;

        EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(keys[i], NULL);
        EVP_PKEY_decrypt_init(ctx);
        EVP_PKEY_decrypt(ctx, NULL, &outl, enc_key[i], enc_key_length[i]);

        out = OPENSSL_malloc(outl);
        EVP_PKEY_decrypt(ctx, out, &outl, enc_key[i], enc_key_length[i]);

        BIO_printf(bio, "Decrypted Session Key(%d)\n", i + 1);
        BIO_dump(bio, (char*)out, outl);

        

        EVP_PKEY_CTX_free(ctx);
        OPENSSL_free(out);
    }

    ////////////////////////////////////////////////////////////////////////////
    // CLEANUP

    for (int i = 0; i < N_KEYS; ++i)
    {
        OPENSSL_free(enc_key[i]);
        EVP_PKEY_free(pubkeys[i]);
        EVP_PKEY_free(keys[i]);
    }
    OPENSSL_free(encrypted);
    OPENSSL_free(enc_key);
    OPENSSL_free(enc_key_length);
    OPENSSL_free(pubkeys);
    OPENSSL_free(keys);
    BIO_free(bio);

    return EXIT_SUCCESS;*/
}