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
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <dirent.h>
#include <pthread.h>

#include "util.h"
#include "filenames.h"

void getStoragePath(char* file_name, char** strg_path) {
    strcpy(*strg_path, "./storage/");
    uint8_t len = strlen(file_name);
    strncat(*strg_path, file_name, len);
}

int fileExists(uint8_t client_fd, char* file_name, char tag) {
    uint8_t len = HEADER_LEN + 1;
    void* packet = malloc(HEADER_LEN + 1);
    void* data = malloc(1);
    int file_exists = access(file_name, F_OK);
    if (file_exists == 0) { //file does exist
        memset(data, 'Y', 1);
    }
    else memset(data, 'N', 1); //file doesn't exist
    constructPacket(len, &tag, data, &packet);
    if (sendAll(client_fd, packet, &len) == -1) {
        printf("Error: was unable to notify client that file exists or not\n");
        return -1;
    }
    free(data);
    return (file_exists == 0) ? 1 : 0;
}

void handleClientDownload(uint8_t client_fd, char* rel_path, struct FileProtocolPacket* client_request) {
    if (!pathIsValid(rel_path))
        printf("Server: client attempted to download file with bad path\n");
    int file_exists = fileExists(client_fd, rel_path, client_request->tag);
    if (file_exists)
        uploadFile(client_fd, rel_path);
    else if (file_exists == -1)
        printf("Server: Error - server unable to notify client on file existence\n");
    else
        printf("Server: client attempted to download file with name that does not exist\n");
}

void handleClientUpload(uint8_t client_fd, char* rel_path, struct FileProtocolPacket* client_request) {
    int file_exists = fileExists(client_fd, rel_path, client_request->tag);
    if (!file_exists)
        downloadFile(client_fd, rel_path);
    else if (file_exists == -1)
        printf("Sever: Error - server unable to notify client on file existence\n");
    else //deny upload
        printf("Server: client attempted to upload file with name that already exists\n");
}

void handleClientGetFilenames(uint8_t client_fd, struct FileProtocolPacket* client_request) {
    if (!serialiseAndSendFilenames(client_fd))
        printf("Server: getFilenames failed to send out all packets to socket %d\n", client_fd);
}

void handleClientRequest(uint8_t client_fd, struct FileProtocolPacket* client_request) {
    char* rel_path;
    int is_get_req = (client_request->length < HEADER_LEN);
    if (!is_get_req) { //not a get request
        const uint8_t d_name_len = 11;
        rel_path = malloc(strlen(client_request->data) + d_name_len);
        getStoragePath(client_request->data, &rel_path);
    }
    switch(client_request->tag) {
        case 'G':
            handleClientGetFilenames(client_fd, client_request);
            break;
        case 'U':
            handleClientUpload(client_fd, rel_path, client_request);
            break;
        case 'D':
            handleClientDownload(client_fd, rel_path, client_request);
            break;
        default:
            printf("server: received invalid packet from client\n");
            break;
    }
    if (!is_get_req) free(rel_path);
    free(client_request);
    client_request = NULL;
}

int handleNewConnection(uint8_t listener_socket) {
    struct sockaddr_storage client_addr;
    char client_ip[INET_ADDRSTRLEN];
    socklen_t addr_size = sizeof(client_addr);
    int new_fd = accept(
        listener_socket,
        (struct sockaddr*)&client_addr,
        &addr_size
    );
    if (new_fd == -1)
        return 0;
    else {
        printf(
            "Server: new connection from %s on "
            "socket %d\n",
            inet_ntop(
                client_addr.ss_family,
                getAddress((struct sockaddr*)&client_addr),
                client_ip,
                INET6_ADDRSTRLEN
            ),
            new_fd
        );
        return new_fd;
    }
}

void* handleClientConnection(void* fd_input) {
    uint8_t client_fd = (uint8_t)fd_input;
    struct FileProtocolPacket* client_request;
    if (!recvAll(client_fd, &client_request)) {
        printf("Server: error in receiving data from client\n");
        close(client_fd);
        pthread_exit(NULL);
    }
    else 
        handleClientRequest(client_fd, &(*client_request));
    close(client_fd);
    pthread_exit(NULL);
}

void startMainLoop(uint8_t listener_socket) {
    pthread_attr_t attr;
    pthread_t thread;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    for (;;) {
        uint8_t client_fd = handleNewConnection(listener_socket);
        if (!client_fd) {
            printf("Server: error with client connecting\n");
            continue;
        }
        pthread_create(&thread, &attr, handleClientConnection, (void*)client_fd);
    }
    pthread_attr_destroy(&attr);
    pthread_exit(NULL);
}

void initialiseServer() {
    uint8_t listener_socket = setupSocket(1, NULL);
    if (listener_socket == -1) {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }
    startMainLoop(listener_socket);
}

int main(void) {
    initialiseServer();
}
