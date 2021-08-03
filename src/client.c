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
#include <libgen.h>

#include "util.h"

void getFileNames(uint8_t tcp_fd) {
    printf("\n[FILES]\n");
    for (;;) {
        struct FileProtocolPacket* resp;
        if (!recvAll(tcp_fd, &resp)) {
            printf("Client: error in receiving file names from server\n");
            exit(1);
        }
        if (!strcmp(resp->filename, "END")) {
            printf("Client: successfully retrieved file list from server\n");
            break;
        }
        printf("%s", resp->filename);
        free(resp);
        resp = NULL;
    }
    printf("\n");
}

int sendRequest(uint8_t tcp_fd, char* tag, char* filename) {
    uint8_t request_len = HEADER_LEN + strlen(filename);
    void* packet = malloc(request_len);
    constructPacket(request_len, tag, filename, &packet);
    if (sendAll(tcp_fd, packet, &request_len) == -1) {
        printf("Error: client was only able to send out %d bytes of the request\n", request_len);
        return 0;
    }
    return 1;
}

int fileExistsOnServer(uint8_t tcp_fd) {
    char buffer[1];
    uint8_t num_bytes = recv(tcp_fd, buffer, sizeof(buffer), 0);
    if (num_bytes == -1) {
        printf("Error: failed to receive response from server that file exists\n");
        return 0;
    }
    if (buffer[0] == 'N') return 0;
    else if (buffer[0] == 'Y') return 1;
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

void handleGetFiles(uint8_t tcp_fd) {
    if (!sendRequest(tcp_fd, "G", "")) 
        failedRequest("get files");
    getFileNames(tcp_fd);            
}

void handleClientDownload(uint8_t tcp_fd, char* filename) {
    if (!sendRequest(tcp_fd, "D", filename)) 
        failedRequest("download file");
    if (fileExistsOnServer(tcp_fd))
        downloadFile(tcp_fd, filename);
    else {
        printf("Error: file with that name does not exist on server\n");
    }
}

void handleClientUpload(uint8_t tcp_fd, char* filename) {
    if (access(filename, F_OK) != 0) {
        printf("Error: file doesn't exist on client\n");
        exit(1);
    }
    if (!sendRequest(tcp_fd, "U", basename(filename)))
        failedRequest("upload file");
    if (fileExistsOnServer(tcp_fd)) {
        printf("Error: file with that name already exists on server\n");
    }
    else uploadFile(tcp_fd, filename);
}

void interpretCommand(uint8_t tcp_fd, char** argv) {
    if (!strcmp(argv[2], "-g"))
        handleGetFiles(tcp_fd);
    else if (!strcmp(argv[2], "-d"))
        handleClientDownload(tcp_fd, argv[3]);
    else if (!strcmp(argv[2], "-u"))
        handleClientUpload(tcp_fd, argv[3]);
    else if (!strcmp(argv[2], "-h"))
       printHelpInfo();
    else 
        inputError();
}

void initialiseClient(int argc, char** argv) {
    if (strcmp(argv[1], "-h") == 0)
        printHelpInfo();
    if (argc < 3 || argc > 4) {
       inputError();
    }
    int tcp_fd = setupSocket(0, argv[1]);
    if (tcp_fd == 0) exit(1);
    interpretCommand(tcp_fd, argv);
}

int main(int argc, char* argv[]) {
    initialiseClient(argc, argv);
    return 0;
}

//./client localhost -u file1.txt
//./client localhost -d file2.txt
//./client localhost -g
//./client localhost -h
