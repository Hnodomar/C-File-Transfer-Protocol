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

int fileExists(const char* file_name) {
    if (access(file_name, F_OK) == 0) //file does exist
        return 1;
    else { //file doesn't exist
        FILE* file = fopen(file_name, "w");
        fclose(file);
        return 0;
    }
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

void* getAddress(struct sockaddr* socket_addr) {
    if (socket_addr->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)socket_addr)->sin_addr);
    }
    return &(((struct sockaddr_in6*)socket_addr)->sin6_addr);
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

int getFilenames(char*** names, size_t names_size) {
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

//Setup TCP connection to get filename and communicate
//if this exists or not. Then, setup UDP socket to transfer
//file on

int main(void) {
    int serv_socket = setupServerSocket();
    if (serv_socket == -1) {
        fprintf(stderr, "error setting up server socket\n");
        exit(1);
    }
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
    char tcp_buffer[256];
    int bytes_received;
    char* file_name;

    struct sigaction signal_action;
    setupSigAction(&signal_action);
    for (;;) {
        int poll_count = poll(pfds, fd_count, -1);
        if (poll_count == -1) {
            perror("poll");
            exit(1);
        }
        for (int i = 0; i < fd_count; ++i) {
            if (pfds[i].revents & POLLIN) {
                if (pfds[i].fd == listener_socket) { //new connection
                    addr_size = sizeof client_addr;
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
                    int num_bytes = recv(pfds[i].fd, tcp_buffer, sizeof tcp_buffer, 0);
                    int sender_fd = pfds[i].fd;
                    if (num_bytes <= 0) {
                        if (num_bytes == 0)
                            printf("server: socket %d hung up\n", sender_fd);
                        else
                            perror("recv");
                        close(pfds[i].fd);
                        delFromPfds(pfds, i, &fd_count);
                    }
                    else { //this is where client sends interface data
                        if (!fork()) {
                            if (tcp_buffer[0] == 'G') { //get filenames
                                size_t files_arr_size = 10;
                                char** file_names = malloc(files_arr_size * sizeof(*file_names));
                                int num_files = getFilenames(&file_names, files_arr_size);
                                if (num_files == 0) {
                                    exit(1);
                                }
                            }
                            
                            if (tcp_buffer[0] == 'U') { //client upload file
                                //rest of buffer should have filename
                            }

                            if (tcp_buffer[0] == 'D') { //client download file
                                //rest of buffer should have filename
                            }
                            close(pfds[sender_fd].fd);
                            close(listener_socket);
                            exit(0);
                        }
                    }
                }
            }
        }
    }



    /*
    for (;;) {
        bytes_received = recvfrom(
            serv_socket,
            buffer,
            BUFF_SIZE,
            0,
            (struct sockaddr*)&client_addr,
            &addr_size
        );
        
        if (fileExists(file_name)) {
            notifyClientFileExists();
            continue;
        }
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
    }*/
}