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
    return sock_fd;
}

int greetClientGetInput() {
    int input;
    printf(
        "Welcome to the file transfer interface\n"
        "Please choose an option:\n"
        "(1) Get files on server\n"
        "(2) Download file from server\n"
        "(3) Upload file to server\n\n"
    );
    scanf("%d", &input);
    if (input != 1 && input != 2 && input != 3) {
        printf("Invalid input. Please try again\n");
    }
    return input;
}

int getFiles(int tcp_fd) {
    if (send(tcp_fd, "G", 1, 0) == -1) {
        perror("Send failed");
        printf("Error: get file request to server failed");
        return 0;
    }
    else {
        int packet_size = 42;
        char buffer[packet_size];
        int num_bytes = recv(tcp_fd, buffer, packet_size - 1, 0);
        if (num_bytes == -1) {
            perror("Receive failed");
            printf("Error: get file response to server failed");
            return 0;
        }
        buffer[num_bytes] = '\0';
        printf("BYTES RECEIVED: %d\n", num_bytes);
        printf(
            "Get files successful!\n"
            "Files on server:\n\n"
            "%s\n\n", buffer
        );
    }
    return 1;
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
    if (tcp_fd == 0) {
        return 1;
    }
    for(;;) {
        int input = greetClientGetInput();
        if (input == 1) { //Get files
            getFiles(tcp_fd);
        }
    }
}
