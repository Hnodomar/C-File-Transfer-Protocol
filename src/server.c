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

int readFilenames(char*** names, size_t names_size, uint16_t* files_str_len) {
    DIR* d;
    struct dirent* dir;
    size_t i = 0;
    d = opendir("./storage");
    int num_names = 0;
    const char* current_dir = ".";
    const char* prev_dir = "..";
    if (d) {
        while((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, prev_dir) == 0 || strcmp(dir->d_name, current_dir) == 0) {
                continue;
            }
            if (num_names == names_size) {
                names_size *= 2;
                char** tmp = (char**)realloc(*names, names_size * sizeof(char*));
                if (tmp != NULL)
                    *names = tmp;
                else
                    return 0;
            }
            (*names)[i] = (char*)malloc(sizeof(dir->d_name) + 1);
            strcpy((*names)[i], dir->d_name);
            *files_str_len += strlen(dir->d_name);
            ++num_names;
            ++i;
        }
        closedir(d);
    }
    return num_names;
}

int serialiseFilenames(char*** names, char** names_serialised, int max_size, int num_files) {
    int serial_length = 0;
    int name_length;
    int files_left = num_files;
    for (int i = 0; i < num_files; ++i) {
        name_length = strlen((*names)[i]);
        if ((serial_length + name_length) > max_size) {
            break;
        }
        serial_length += name_length;
        strcat((*names_serialised), (*names)[i]);
        strcat((*names_serialised), "\n");
        --files_left;
        if (files_left == 0) {
            strcat((*names_serialised), "END\n");
        }
    }
    return files_left;
}

int getFilenames(uint8_t sender_fd) {
    size_t files_arr_size = 10;
    char** file_names = malloc(files_arr_size * sizeof(*file_names));
    //TODO: implement file-name caching
    //TODO: get full length of list, include this in packet sent to client
    uint16_t files_str_len;
    uint8_t num_files = readFilenames(&file_names, files_arr_size, &files_str_len);
    files_str_len += num_files;
    files_str_len += 4;
    printf("FILES STR LEN: %d\n", files_str_len);
    if (num_files == 0) {
        printf("server: failed to read filenames, realloc failed");
        exit(1);
    }
    else {
        uint8_t num_left = num_files;
        uint8_t serial_size = 254;
        while (num_left > 0) {
            char* files_serialised = malloc(serial_size);
            strcpy(files_serialised, "\0");
            num_left = serialiseFilenames(
                &file_names, 
                &files_serialised, 
                serial_size, 
                num_left
            );
            //send TCP packet to client with filenames
            printf("\nserialised string:\n\n%s", files_serialised);
            printf("strlen: %ld\n", strlen(files_serialised));
            uint8_t pkt_len = strlen(files_serialised) + 2;
            void* pkt = malloc(pkt_len);
            char tag = 'G';
            constructPacket(pkt_len, &tag, (void*)files_serialised, &pkt);
            if (sendAll(sender_fd, pkt, &pkt_len) == -1) {
                printf("server: sendAll() of filenames getting failed\n");
                return 0;
            }
            free(pkt);
            free(files_serialised);
        }
    }
    close(sender_fd);
    return 1;
}

void handleClientRequest(uint8_t client_fd, struct FileProtocolPacket* client_request) {
    printf("length: %d\n", client_request->length);
    printf("tag: %c\n", client_request->tag);
    printf("filename: %s\n", client_request->filename);
    char* rel_path;
    if (client_request->length > 2) {
        rel_path = malloc(strlen(client_request->filename) + 11);
        getStoragePath(client_request->filename, &rel_path);
    }
    switch(client_request->tag) {
        case 'G':
            if (!getFilenames(client_fd))
                printf("server: getFilenames failed to send out all packets\n");
            break;
        case 'U': {
            if (!fileExists(client_fd, rel_path)) {
                downloadFile(client_fd, rel_path);
            }
            else //deny upload
                printf("Server: client attempted to upload file with name that already exists\n");
            break;
        }
        case 'D':
            if (fileExists(client_fd, rel_path)) {
                uploadFile(client_fd, rel_path);
            }
            else
                printf("Server: client attempted to download file with name that does not exist\n");
            break;
        default:
            printf("server: received invalid packet from client\n");
            break;
    }
    free(client_request);
}

uint8_t handleNewConnection(uint8_t listener_socket) {
    struct sockaddr_storage client_addr;
    char client_ip[INET_ADDRSTRLEN];
    socklen_t addr_size = sizeof(client_addr);
    uint8_t new_fd = accept(
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
