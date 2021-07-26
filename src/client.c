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
    system("clear");
    if (input != 1 && input != 2 && input != 3) {
        printf("Invalid input. Please try again\n");
        return 4;
    }
    return input;
}

struct ServerResponse {
    uint8_t length;
    char response_text[254];
};

void getFiles(uint8_t tcp_fd) {
    uint8_t packet_size = 255;
    void* buffer = malloc(packet_size);
    uint8_t num_bytes = recv(tcp_fd, buffer, packet_size, 0);
    struct ServerResponse* resp = (struct ServerResponse*)buffer; 
    if (num_bytes == -1) {
        perror("Receive failed");
        printf("Error: get file response to server failed");
        return;
    }
    printf("STR: %s\n", resp->response_text);
}

char* getFilename(int tcp_fd) {
    char* filename;
    while(1) {
        printf("Enter filename: \n");
        char str[300];
        scanf("%s", str);
        if (strlen(str) > 254) {
            printf("Error: filename too long. Try again\n");
        }
        else {
            filename = malloc(strlen(str));
            strcpy(filename, str);
            break; 
        }
    }
    return filename;
}

int sendRequest(uint8_t tcp_fd, char* tag, char* filename) {
    printf("str: %s\n", filename);
    uint8_t file_len = strlen(filename);
    uint8_t packet_length = 2 + file_len;
    void* packet = malloc(packet_length);
    memcpy(packet, &packet_length, 1);
    memcpy(packet + 1, (const void*)tag, 1);
    if (strcmp("", filename) != 0)
        memcpy(packet + 2, filename, file_len);
    if (sendAll(tcp_fd, packet, &packet_length) == -1) {
        printf("Error: client was only able to send out %d bytes of the request\n", packet_length);
        return 0;
    }
    return 1;
}

uint8_t fileExistsOnServer(uint8_t tcp_fd) {
    char buffer[1];
    uint8_t num_bytes = recv(tcp_fd, buffer, sizeof(buffer), 0);
    if (num_bytes == -1) {
        printf("Error: failed to receive response from server that file exists\n");
        return 0;
    }
    if (buffer[0] == 'N') return 0;
    else if (buffer[0] == 'Y') return 1;
}

int uploadFileToServer(uint8_t tcp_fd) {

}

int downloadFileFromServer(uint8_t tcp_fd) {

}

void startMainLoop(uint8_t tcp_fd) {
    for(;;) {
        uint8_t input = greetClientGetInput();
        if (input == 1) { 
            if (!sendRequest(tcp_fd, "G", ""))
                continue;
            getFiles(tcp_fd);            
        }
        else if (input == 2) {
            if (!sendRequest(tcp_fd, "D", getFilename(tcp_fd)))
                continue;
            if (fileExistsOnServer(tcp_fd) == 1)
                downloadFileFromServer(tcp_fd);
            else {
                printf("Error: file with that name does not exist on server\n");
                continue;
            }
        }
        else if (input == 3) {
            if (!sendRequest(tcp_fd, "U", getFilename(tcp_fd)))
                continue;
            if (fileExistsOnServer(tcp_fd)) {
                printf("Error: file with that name already exists on server\n");
                continue;
            }
            else uploadFileToServer(tcp_fd);
        }
    }
}

int initialiseClient(int argc, char** argv[]) {
    if (argc != 2) {
        fprintf(
            stderr, 
            "Error: not called with correct number of arguments\n"
            "Usage: fileTransferClient [hostname]\n"
        );
        exit(1);
    }
    uint8_t tcp_fd = connectToServer((*argv)[1]);
    if (tcp_fd == 0) return 0;
    startMainLoop(tcp_fd);
}

int main(int argc, char* argv[]) {
    if (!initialiseClient(argc, &argv))
        return 1;
}
