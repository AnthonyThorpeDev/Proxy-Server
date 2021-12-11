#ifndef NETWORK_H
#define NETWORK_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h> /* for looking up hosts via DNS (struct hostent, gethostbyname()) */

#define WEB_PORT 80
#define GET_REQ "GET "
#define POST_REQ "POST "
#define PUT_REQ "PUT "

#define HTTP_FRAME "http://"
#define WWW_FRAME "www."
#define DOT_COM ".com"
#define DOT_ORG ".org"
#define DOT_EDU ".edu"
#define DOT_GOV ".gov"
#define DOT_NET ".net"

enum REQUEST_TYPE {
    GET = 1,
    POST,
    PUT
};

void dump(char *str) {
    printf("%s", str);
    //printf("\n");
    int i;
    for(i = 0; i < strlen(str); i++) {
        printf("%02x ", str[i]);
    }
    printf("\n");
}

#endif
