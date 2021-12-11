#include "common.h"
#include "networking.h"

typedef struct P2H {
    char request[10];
    char host[200];
    char resource[200];
    char HTTP_type[10];
} Proxy2Host;

void execute_request(char *resource, char *host, char *request_type, char *HTTP_type, char *headers, int c2h_fd);
char *form_request(char *request, char *resource, char *HTTP_type, char *host, char *headers); // TODO

int main(int argc, char **argv) {
    if(argc != 2) {
        usage(argv[0], "Port number");
    }

    /** port number of web proxy that client connects to */
    int port = atoi(argv[1]);

    /** socket that will listen to client HTTP requests */
    int proxy_listener = socket(AF_INET, SOCK_STREAM, 0);
    if(proxy_listener == -1) {
        err("creating socket");
    }

    /** resuse socket. This allows socket to bind to a given port
     * even if port is seemingly already in use (if socket not closed properly)
     */
    int optval = 1;
    if(setsockopt(proxy_listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1) {
        err("setting socket option");
    }

    /** address of web proxy. This will take in requests from client, send them to
        remote target, receive response from remote host, and send the response to the client */
    struct sockaddr_in proxy;
    memset(&proxy, 0, sizeof(proxy));
    proxy.sin_family = AF_INET;
    proxy.sin_addr.s_addr = INADDR_ANY;
    proxy.sin_port = htons(port);

    /* bind the c2p socket to address of proxy */
    if(bind(proxy_listener, (struct sockaddr *) &proxy, sizeof(proxy)) == -1) {
        err("binding");
    }

    /* tell the socket to listen to requests on proxy's address */
    if(listen(proxy_listener, 1) == -1) {
        err("listening");
    }

    /* start up the proxy server */
    char recv_buffer[5000], send_buffer[5000];
    while(1) {
        int client_to_proxy;
        socklen_t len = sizeof(struct sockaddr);
        struct sockaddr_in client;

        printf("listening on %s on port: %d\n", inet_ntoa(proxy.sin_addr), ntohs(proxy.sin_port));

        /* accept a client request to the proxy server */
        client_to_proxy = accept(proxy_listener, (struct sockaddr *) &client, &len);
        if(client_to_proxy == -1) {
            err("accepting client request to proxy");
        }
        
        memset(recv_buffer, 0, sizeof(recv_buffer));
        if(recv(client_to_proxy, recv_buffer, sizeof(recv_buffer) - 1, 0) == -1) {
            err("receiving data from client");
        }
        printf("\nCLIENT REQUEST:\n%s\n",recv_buffer);

        // copy of the request
        char *req_copy = strdup(recv_buffer);

        // get the first line of the request
        char *req_token = strtok(req_copy, "\n");

        // proxy to host request
        Proxy2Host p2h;

        // set ptr to the first line of the request
        char *ptr = req_token;

        // get the request type (GET, POST, etc)
        int i;
        for(i = 0; !isspace(ptr[i]); i++) {
            p2h.request[i] = ptr[i];
        }

        // getting the host URL
        char *host_ptr = req_token + strlen(p2h.request) + 2;
        int fwdslash = 0;
        for(i = 0; !isspace(host_ptr[i]); i++) {
            if(fwdslash == 2)
                break;
            if(host_ptr[i] == '/') {
                fwdslash++;
            }
            p2h.host[i] = host_ptr[i];
        }

        for(; !isspace(host_ptr[i]) && host_ptr[i] != '/'; i++) {
            p2h.host[i] = host_ptr[i];
        }
        p2h.host[i] = 0;
       // printf("host: %s\n", p2h.host);
        
        // getting the resource
        int j;
        for(j = 0; !isspace(host_ptr[i]); i++, j++) {
            p2h.resource[j] = host_ptr[i];
        }
        p2h.resource[j] = 0;
        
       // printf("resource: %s\n", p2h.resource);

        // getting the HTTP type
        char *http_type = strstr(req_token, "HTTP/1.");
        memset(p2h.HTTP_type, 0, sizeof(p2h.HTTP_type));
        strncpy(p2h.HTTP_type, http_type, 8);

        req_token = strtok(NULL, "");

       // printf("REST of request:\n %s\n", req_token);

        char *rest_ptr = req_token;
        if(rest_ptr) {
            while(*rest_ptr != '\n') {
                rest_ptr++;
            }
        }
        

        char *headers = rest_ptr;

        free(req_copy);

        execute_request(p2h.resource, p2h.host, p2h.request, p2h.HTTP_type, headers, client_to_proxy);
        // NEED TO DELETE THIS BREAK EVENTUALLY
        break;
    }

    close(proxy_listener);
    return EXIT_SUCCESS;
}

char *form_request(char *request_type, char *resource, char *HTTP_type, char *host, char *headers) {
    char *host_to_send = NULL;
    if((host_to_send = strstr(host, "http://")) || (host_to_send = strstr(host, "https://"))) {
        if((host_to_send = strstr(host, "http://"))) {
            host_to_send = host + 7;
        } else {
            host_to_send = host + 8;
        }
    }

    char *req_return = NULL;
    if(headers) {
        req_return = (char *) malloc(strlen(request_type) + strlen(resource) + 
        strlen(HTTP_type) + strlen("\nHost: ") + strlen(host) + strlen(headers) + 4); // + 4 for the '\n', \r\n, and the '\0

    } else {
        req_return = (char *) malloc(strlen(request_type) + strlen(resource) + 
        strlen(HTTP_type) + strlen("\nHost: ") + strlen(host) + 4); // + 4 for the '\n', \r\n, and the '\0
    }
    
    strcat(req_return, request_type);
    strcat(req_return, " ");
    strcat(req_return, resource);
    strcat(req_return, " ");
    strcat(req_return, HTTP_type);
    strcat(req_return, "\nHost: ");
    strcat(req_return, host_to_send);
    if(headers) {
        strcat(req_return, headers);
    }
    strcat(req_return, "\r\n");

    return req_return;
}

// TODO
void execute_request(char *resource, char *host, char *request_type, char *HTTP_type, char *headers, int c2h_fd) {
    /* gethostbyname() returns a struct hostent * */
    struct hostent *target_info = NULL;

    /* remote host address that sock_fd will connect to */
    struct sockaddr_in remote_host;

    /* socket that will connect to the address provided by target_info */
    int sock_fd;

    /* extract the DNS for use in gethostbyname() */
    char send_buf[5000];
    memset(send_buf, 0, sizeof(send_buf));

    /* chop off https:// or http:// for gethostbyname() */
    char *host_ex_http = NULL;
    if((host_ex_http = strstr(host, "http://")) || (host_ex_http = strstr(host, "https://"))) {
        if((host_ex_http = strstr(host, "http://"))) {
            host_ex_http = host + 7;
        } else {
            host_ex_http = host + 8;
        }
    }
    /* look up the host */
    target_info = gethostbyname(host_ex_http);
    if(!target_info) {
        printf("Host: %s, could not be found\n", host);
        return;
    } 

    /* set up request to send to remote host */
    if(strlen(resource) == 0) {
        resource = "/"; // if there is no resource, set it to '/'
    }
    
    char *request_to_send = form_request(request_type, resource, HTTP_type, host, headers);

    printf("SENDING:\n");
    dump(request_to_send);

    /* fill in the remote host information */
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd == -1) {
        printf("Failed to create socket to remote host\n");
        return;
    }
    memset(&remote_host, 0, sizeof(remote_host));
    remote_host.sin_addr = *(struct in_addr *) target_info->h_addr_list[0];
    remote_host.sin_family = AF_INET;
    remote_host.sin_port = htons(WEB_PORT);

    
    // connect to remote host
    if(connect(sock_fd, (struct sockaddr *) &remote_host, sizeof(remote_host)) == -1) {
        printf("Failed to connect to remote host\n");
        return;
    } 

    // send request to remote host
    if(send(sock_fd, request_to_send, strlen(request_to_send), 0) == -1) {
        printf("Failed to send request to remote host\n");
        return;
    }
    

    char recv_buf[5000];
    memset(recv_buf, 0, sizeof(recv_buf));

    if(recv(sock_fd, recv_buf, sizeof(recv_buf), 0) == -1) {
        printf("Failed to receive data from remote host\n");
        return;
    }

    printf("\nRECEIVED: \n%s\n", recv_buf);

    /* send the response to the client (browser?) */
    if(send(c2h_fd, recv_buf, sizeof(recv_buf), 0) == -1) {
        printf("Failed to send response to client\n");
        return;
    }

    close(sock_fd);
    free(request_to_send);
}
