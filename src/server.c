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

#include "util.h"
#include "filenames.h"

#define PORT "9034"
#define BUFF_SIZE 1024

int setupListenerSocket(void) {
    struct addrinfo hints, *a_info, *a_ele;
    int listener;
    int yes = 1;
    int addr_return;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    addr_return = getaddrinfo(NULL, PORT, &hints, &a_info);
    if (addr_return == -1) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(addr_return));
        exit(1);
    }

    for (a_ele = a_info; a_ele != NULL; a_ele = a_ele->ai_next) {
        listener = socket(
            a_ele->ai_family, 
            a_ele->ai_socktype, 
            a_ele->ai_protocol
        );
        if (listener < 0 ) {
            continue;
        }
        setsockopt( //get rid of "address already in use" error msg
            listener,
            SOL_SOCKET,
            SO_REUSEADDR,
            &yes,
            sizeof(int)
        );
        if (bind(listener, a_ele->ai_addr, a_ele->ai_addrlen) == -1) {
            close(listener);
            continue;
        }
        break;
    }
    freeaddrinfo(a_info);
    if (a_ele == NULL) {
        return -1;
    }
    if (listen(listener, 10) == -1) {
        return -1;
    }
    return listener;
}

void getStoragePath(char* file_name, char** strg_path) {
    strcpy(*strg_path, "./storage/");
    uint8_t len = strlen(file_name);
    strncat(*strg_path, file_name, len);
}

uint8_t fileExists(uint8_t client_fd, char* file_name) {
    if (access(file_name, F_OK) == 0) { //file does exist
        printf("file exists\n");
        send(client_fd, "Y", 1, 0);
        return 1;
    }
    send(client_fd, "N", 1, 0); //file doesn't exist
    printf("file doesnt exist\n");
    return 0;
}

void signalChildHandler(int signal) {
    int saved_errno = errno; //waitpid might overwrite this
    int child_pid;
    while((child_pid = waitpid(-1, NULL, WNOHANG)) > 0)
        printf("Child process terminated, PID: %d\n", child_pid);
    errno = saved_errno;
}

void setupSigAction(struct sigaction* signal_action) {
    signal_action->sa_handler = signalChildHandler;
    sigemptyset(&(signal_action->sa_mask));
    signal_action->sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &(*signal_action), NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
}

void handleClientDownload(uint8_t client_fd, char* rel_path, struct FileProtocolPacket* client_request) {
    if (fileExists(client_fd, rel_path))
        uploadFile(client_fd, rel_path);
    else
        printf("Server: client attempted to download file with name that does not exist\n");
}

void handleClientUpload(uint8_t client_fd, char* rel_path, struct FileProtocolPacket* client_request) {
    if (!fileExists(client_fd, rel_path))
        downloadFile(client_fd, rel_path);
    else //deny upload
        printf("Server: client attempted to upload file with name that already exists\n");
}

void handleClientGetFilenames(uint8_t client_fd, struct FileProtocolPacket* client_request) {
    if (!serialiseAndSendFilenames(client_fd))
        printf("Server: getFilenames failed to send out all packets to socket %d\n", client_fd);
}

void handleClientRequest(uint8_t client_fd, struct FileProtocolPacket* client_request) {
    char* rel_path;
    if (client_request->length > 2) { //not a get request
        const uint8_t d_name_len = 11;
        rel_path = malloc(strlen(client_request->filename) + d_name_len);
        getStoragePath(client_request->filename, &rel_path);
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

void startMainLoop(uint8_t listener_socket) {
    struct sigaction signal_action;
    setupSigAction(&signal_action);
    for (;;) {
        uint8_t client_fd = handleNewConnection(listener_socket);
        if (!client_fd) {
            printf("Server: error with client connecting\n");
            continue;
        }
        if (!fork()) {
            close(listener_socket);
            struct FileProtocolPacket* client_request;
            if (!recvAll(client_fd, &client_request)) {
                printf("Server: error in receiving data from client\n");
                exit(1);
            }
            else 
                handleClientRequest(client_fd, &(*client_request));
            close(client_fd);
            exit(0);
        }
        close(client_fd);
    }
}

void initialiseServer() {
    uint8_t listener_socket = setupListenerSocket();
    if (listener_socket == -1) {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }
    startMainLoop(listener_socket);
}

int main(void) {
    initialiseServer();
}
