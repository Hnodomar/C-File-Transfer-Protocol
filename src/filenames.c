#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "filenames.h"
#include "util.h"

int sendFilenamesToClient(uint8_t client_fd, const uint8_t serialised_len, char* serialised_str) {
    char tag = 'G';
    uint8_t packet_len = strlen(serialised_str) + HEADER_LEN;
    void* packet = malloc(packet_len);
    constructPacket(packet_len, &tag, serialised_str, &packet);
    if (sendAll(client_fd, packet, &packet_len) == -1) {
        printf("Server: error whilst sending filenames to client\n");
        free(serialised_str);
        return 0;
    }
    bzero(serialised_str, serialised_len);
    free(packet);
    return 1;
}

int serialiseAndSendFilenames(uint8_t client_fd) {
    DIR* dir = opendir("./storage");
    if (dir == NULL) return 0;
    struct dirent* dir_file;
    const char* current_dir = ".";
    const char* prev_dir = "..";
    const uint8_t serialised_len = 254;
    uint8_t current_len = 0;
    if (dir) {
        char* serialised_str = malloc(serialised_len);
        while((dir_file = readdir(dir)) != NULL) {
            const char* filename = dir_file->d_name;
            if (!strcmp(filename, current_dir) || !strcmp(filename, prev_dir)) {
                continue;
            }
            uint8_t name_len = strlen(filename);
            if ((current_len + name_len + 1) > serialised_len) {
                if (!sendFilenamesToClient(client_fd, serialised_len, serialised_str))
                    return 0;
                current_len = 0;
            }
            strncpy(serialised_str + current_len, filename, strlen(filename));
            strncat(serialised_str, "\n", 1);
            current_len += (name_len + 1);
        }
        if (current_len != 0)
            if (!sendFilenamesToClient(client_fd, serialised_len, serialised_str))
                return 0;
        free(serialised_str);
        closedir(dir);
        notifyFullFileSent(client_fd, 'G');
        return 1;
    }
    else {
        closedir(dir);
        printf("Server: error whilst trying to open storage directory to read filenames\n");
        return 0;
    }
}
