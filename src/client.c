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

void getFiles(uint8_t tcp_fd) {
    uint8_t packet_size = 255;
    char* filename_str = malloc(packet_size);
    uint16_t filename_str_size = packet_size;
    uint16_t filename_str_len = 0;
    strcpy(filename_str, "\0");
    for (;;) {
        struct FileProtocolPacket* resp;
        if (!recvAll(tcp_fd, &resp)) {
            printf("Client: error in receiving file names from server\n");
            exit(1);
        }
        char substr[3];
        memcpy(substr, &(resp->filename[resp->length-6]), 3);
        if ((strlen(filename_str) + (resp->length - 2)) > packet_size) {
            filename_str_size *= 2;
            char* tmp = realloc(filename_str, filename_str_size);
            if (tmp != NULL)
                filename_str = tmp;
            else {
                printf("realloc failure!\n");
                return;
            }
        }
        strncat(filename_str, resp->filename, resp->length - 2);
        filename_str_len += (resp->length - 2);
        free(resp);
        if (!strcmp(substr, "END")) break;
    }
    printf(
        "Files successfully retrieved from server!\n"
        "%s\n", filename_str
    );
    free(filename_str);
}

char* getFilename() {
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
    uint8_t request_len = 2;
    void* packet = malloc(request_len);
    constructPacket(request_len, tag, filename, &packet);
    if (sendAll(tcp_fd, packet, &request_len) == -1) {
        printf("Error: client was only able to send out %d bytes of the request\n", request_len);
        return 0;
    }
    free(packet);
    return 1;
}

int fileExistsOnServer(uint8_t tcp_fd) {
    char buffer[1];
    uint8_t num_bytes = recv(tcp_fd, buffer, sizeof(buffer), 0);
    if (num_bytes == -1) {
        printf("Error: failed to receive response from server that file exists\n");
        return 0;
    }
    printf("buffer contains: %s\n", buffer);
    if (buffer[0] == 'N') return 0;
    else if (buffer[0] == 'Y') return 1;
}

int uploadFileToServer(uint8_t tcp_fd) {

}

int downloadFileFromServer(uint8_t tcp_fd) {

}

void failedRequest(char* req_name) {
    printf("Client: failed to successfully send %s request to server\n", req_name);
    exit(1);
}

void printHelpInfo() {
    printf(
        "#################################################\n"
        "This is the client for a file transfer protocol\n"
        "Call with the following syntax\n"
        "#################################################\n"
        "[Get list of files from server]\n"
        "./Client hostname -g\n"
        "[Download file from server]\n"
        "./Client hostname -d filename\n"
        "[Upload file to server]\n"
        "./Client hostname -u filename\n\n"
    );
    exit(0);
}

void inputError() {
    fprintf(
        stderr,
        "Error: client called with wrong argument\n"
        "Call client with -h flag for more info\n"
    );
    exit(1);
}

void startMainLoop(uint8_t tcp_fd, char** argv) {
    if (!strcmp(argv[2], "-g")) { //get server files list
        if (!sendRequest(tcp_fd, "G", "")) 
            failedRequest("get files");
        getFiles(tcp_fd);            
    }
    else if (!strcmp(argv[2], "-d")) { //download file from server
        if (!sendRequest(tcp_fd, "D", getFilename())) 
            failedRequest("download file");
        if (fileExistsOnServer(tcp_fd))
            downloadFileFromServer(tcp_fd);
        else {
            printf("Error: file with that name does not exist on server\n");
        }
    }
    else if (!strcmp(argv[2], "-u")) { //upload file to server
        if (!sendRequest(tcp_fd, "U", getFilename()))
            failedRequest("upload file");
        if (fileExistsOnServer(tcp_fd)) {
            printf("Error: file with that name already exists on server\n");
        }
        else uploadFileToServer(tcp_fd);
    }
    else if (!strcmp(argv[2], "-h")) {
       printHelpInfo();
    }
    else inputError();
}

void initialiseClient(int argc, char** argv) {
    if (strcmp(argv[1], "-h") == 0)
        printHelpInfo();
    if (argc < 3 || argc > 4) {
       inputError();
    }
    uint8_t tcp_fd = connectToServer(argv[1]);
    if (tcp_fd == 0) exit(1);
    startMainLoop(tcp_fd, argv);
}

int main(int argc, char* argv[]) {
    initialiseClient(argc, argv);
}

//./client localhost -u file1.txt
//./client localhost -d file2.txt
//./client localhost -g
//./client localhost -h
