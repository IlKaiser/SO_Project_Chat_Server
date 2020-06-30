#include<string.h>
#include<stdio.h>
#include "Common/common.h"
int main(){
    char* enc="Stringa **********";
    char benc[100];
    char bdec[100];
    strcpy(benc,base64encode(enc,strlen(enc)));
    strcpy(bdec,base64decode(benc,strlen(benc)));
    printf("Decoded string %s\n",bdec);
}