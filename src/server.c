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

int fileExists(uint8_t client_fd, const char* file_name) {
    if (access(file_name, F_OK) == 0) { //file does exist
        send(client_fd, "Y", 1, 0);
        return 1;
    }
    send(client_fd, "N", 1, 0); //file doesn't exist
    return 0;
}

int notifyClientFileExists() {

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

void addToPfds(struct pollfd* pfds[], int new_fd, int* fd_count, int* fd_size) {
    if (*fd_count == *fd_size) {
        *fd_size *= 2;
        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }
    (*pfds)[*fd_count].fd = new_fd;
    (*pfds)[*fd_count].events = POLLIN;
    ++(*fd_count);
}

void delFromPfds(struct pollfd pfds[], int i, int* fd_count) {
    pfds[i] = pfds[((*fd_count)--) - 1];
}

int readFilenames(char*** names, size_t names_size) {
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

int getFilenames(int sender_fd) {
    size_t files_arr_size = 10;
    char** file_names = malloc(files_arr_size * sizeof(*file_names));
    //TODO: implement file-name caching
    int num_files = readFilenames(&file_names, files_arr_size);
    if (num_files == 0) {
        exit(1);
    }
    else {
        int num_left = num_files;
        int serial_size = 256;
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
            printf("strlen: %d\n", strlen(files_serialised));
            int bytes_sent = send(
                sender_fd,
                files_serialised,
                strlen(files_serialised),
                0
            );
            if (bytes_sent == -1) {
                perror("send");
            }
            //realloc(files_serialised, 0);
            free(files_serialised);
        }
    }
    close(sender_fd);
    return 0;
}

int getFilenameFromClient(char** filename, int length) {

}

struct RequestPacket {
    uint8_t length;
    char tag;
    char filename[253];
};

int main(void) {
    int listener_socket = setupListenerSocket();
    if (listener_socket == -1) {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }
    int fd_count = 0;
    int fd_size = 5;
    struct pollfd* pfds = malloc(sizeof(*pfds) * fd_size); //poll file-descriptors
    pfds[0].fd = listener_socket;
    pfds[0].events = POLLIN;
    ++fd_count;

    struct sockaddr_storage client_addr;
    socklen_t addr_size;
    int new_fd;
    char client_ip[INET_ADDRSTRLEN];

    char file_buffer[BUFF_SIZE];
    int bytes_received;
    char* file_name;

    struct sigaction signal_action;
    setupSigAction(&signal_action);
    for (;;) {
        uint8_t poll_count = poll(pfds, fd_count, -1);
        if (poll_count == -1) continue;
        for (int i = 0; i < fd_count; ++i) {
            if (pfds[i].revents & POLLIN) {
                if (pfds[i].fd == listener_socket) { //new connection
                    addr_size = sizeof(client_addr);
                    new_fd = accept(
                        listener_socket,
                        (struct sockaddr*)&client_addr,
                        &addr_size
                    );
                    if (new_fd == -1)
                        perror("accept");
                    else {
                        addToPfds(&pfds, new_fd, &fd_count, &fd_size);
                        printf(
                            "server: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(
                                client_addr.ss_family,
                                getAddress((struct sockaddr*)&client_addr),
                                client_ip,
                                INET6_ADDRSTRLEN
                            ),
                            new_fd
                        );
                    }
                }
                else { //client has sent data to socket representing client connection
                    void* buffer = malloc(256);
                    uint8_t num_bytes = recv(pfds[i].fd, buffer, 255, 0);
                    uint8_t client_fd = pfds[i].fd;
                    if (num_bytes <= 0) {
                        if (num_bytes == 0)
                            printf("server: socket %d hung up\n", client_fd);
                        else
                            perror("recv");
                        close(pfds[i].fd);
                        delFromPfds(pfds, i, &fd_count);
                    }
                    else { //this is where client sends interface data
                        if (!fork()) {
                            struct RequestPacket* client_request = (struct RequestPacket*)buffer;
                            printf("length: %d\n", client_request->length);
                            printf("tag: %c\n", client_request->tag);
                            printf("filename: %s\n", client_request->filename);
                            switch(client_request->tag) {
                                case 'G':
                                    getFilenames(client_fd);
                                    break;
                                case 'U':
                                    if (!fileExists(client_fd, client_request->filename)) {
                                        //allow upload
                                    }
                                    else //deny upload
                                    break;
                                case 'D':
                                    if (fileExists(client_fd, client_request->filename)) {
                                        //allow download
                                    }
                                    else //deny download
                                    break;
                                default:
                                    printf("server: received invalid packet from client\n");
                                    break;
                            }
                            exit(0);
                        }
                    }
                }
            }
        }
    }
}