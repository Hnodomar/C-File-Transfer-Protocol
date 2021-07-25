#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>



#include "util.h"

#define PORT "9034"

int connectToServer(const char* hostname) {
    struct addrinfo hints, *a_info, *a_ele;
    int addr_return, sock_fd, yes = 1;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addr_return = getaddrinfo(hostname, PORT, &hints, &a_info);
    if (addr_return == -1) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addr_return));
    }
    for (a_ele = a_info; a_ele != NULL; a_ele = a_ele->ai_next) {
        sock_fd = socket(
            a_ele->ai_family, 
            a_ele->ai_socktype, 
            a_ele->ai_protocol
        );
        if (sock_fd < 0) continue;
        if (connect(sock_fd, a_ele->ai_addr, a_ele->ai_addrlen) == -1) {
            close(sock_fd);
            continue;
        }
        break;
    }
    if (a_ele == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 0;
    }
    char ip[INET6_ADDRSTRLEN];
    inet_ntop(
        a_ele->ai_family,
        getAddress((struct sockaddr*)a_ele->ai_addr),
        ip,
        sizeof(ip)
    );
    printf("Client: connecting to file server at %s\n", ip);
    freeaddrinfo(a_info);
}



int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(
            stderr, 
            "Error: not called with correct number of arguments\n"
            "Usage: fileTransferClient [hostname]\n"
        );
        exit(1);
    }
    int tcp_fd = connectToServer(argv[1]);
    
}