#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

#define PORT "9034"
#define BUFF_SIZE 1024

int setupServerSocket(void) {
    struct addrinfo hints, *a_info, *a_ele;
    int addr_return;
    int sockfd;
    int yes = 1;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    addr_return = getaddrinfo(NULL, PORT, &hints, &a_info);
    if (addr_return == -1) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addr_return));
        return 1;
    }

    for (a_ele = a_info; a_ele != NULL; a_ele = a_ele->ai_next) {
        sockfd = socket(
            a_ele->ai_family,
            a_ele->ai_socktype,
            a_ele->ai_protocol
        );
        if (sockfd < 0) continue;
        setsockopt(
            sockfd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &yes,
            sizeof(int)
        );
        if (bind(sockfd, a_ele->ai_addr, a_ele->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }
        break;
    }
    freeaddrinfo(a_info);
    if (a_ele == NULL) {
        fprintf(stderr, "server: failed to setup or bind socket\n");
        return -1;
    }
    printf("Datagram socket successfully setup to listen on port %s", PORT);
    return sockfd;
}

int getFileName(void) {

}

struct C_String {
    char* str;
};

int main(void) {
    int serv_socket = setupServerSocket();
    if (serv_socket == -1) {
        fprintf(stderr, "error setting up server socket\n");
        exit(1);
    }

    struct sockaddr_in client_addr;
    socklen_t addr_size;
    char buffer[BUFF_SIZE];
    int bytes_received;

    for (;;) {
        bytes_received = recvfrom(
            serv_socket,
            buffer,
            BUFF_SIZE,
            0,
            (struct sockaddr*)&client_addr,
            &addr_size
        );
        if (bytes_received == 0) continue;
        if (buffer[0] == 'G') { //get filenames
            if (!fork()) {
                close(serv_socket);
            }
        }
        //if upload or download, save filename without forking
        //unsafe to have multiple processes reading/writing to same file
        else if (buffer[0] == 'U') { //upload file
            if (!fork()) {
                close(serv_socket);
                //receive file from client
            }
        }
        else if (buffer[0] == 'D') { //download file
            if (!fork()) {
                close(serv_socket);
                //send file to client
            }
        }      
    }
}