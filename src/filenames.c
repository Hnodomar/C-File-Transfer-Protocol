#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "filenames.h"
#include "util.h"

int findPattern(char* path, char* pattern) {
    int path_itr, pattern_itr;
    int path_len = strlen(path);
    int pattern_len = strlen(pattern);
    if (pattern_len > path_len)
        return -1;
    for (path_itr = 0; path_itr <= path_len - pattern_len; ++path_itr) {
        int position = path_itr;
        int substr_itr = path_itr;
        for (pattern_itr = 0; pattern_itr < pattern_len; ++pattern_itr) {
            if (pattern[pattern_itr] == path[substr_itr])
                ++substr_itr;
            else break;
        }
        if (pattern_itr == pattern_len)
            return position;
    }
    return -1;
}

int pathIsValid(char* path) {
    if (findPattern(path, "..") != -1)
        return 0;
    if (findPattern(path, "#!/bin/bash") != -1)
        return 0;
    return 1;
}

int sendFilenamesToClient(uint8_t client_fd, const uint8_t serialised_len, char* serialised_str) {
    char tag = 'G';
    uint8_t packet_len = strlen(serialised_str) + HEADER_LEN;
    void* packet = malloc(packet_len);
    constructPacket(packet_len, &tag, serialised_str, &packet);
    if (sendAll(client_fd, packet, &packet_len) == -1) {
        printf("Server: error whilst sending filenames to client\n");
        return 0;
    }
    bzero(serialised_str, serialised_len);
    return 1;
}

int serialiseAndSendFilenames(uint8_t client_fd) {
    DIR* dir = opendir("./storage");
    if (dir == NULL) return 0;
    struct dirent* dir_file;
    const char* current_dir = ".";
    const char* prev_dir = "..";
    const uint8_t serialised_len = 253;
    uint16_t current_len = 0;
    if (dir) {
        char* serialised_str = malloc(serialised_len);
        while((dir_file = readdir(dir)) != NULL) {
            const char* filename = dir_file->d_name;
            if (!strcmp(filename, current_dir) || !strcmp(filename, prev_dir)) {
                continue;
            }
            uint16_t name_len = strlen(filename);
            if ((current_len + name_len + 1) > serialised_len) {
                if (!sendFilenamesToClient(client_fd, serialised_len, serialised_str)) {
                    printf("Server: error whilst sending filenames to client!\n");
                    return 0;
                }
                current_len = 0;
            }
            strncpy(serialised_str + current_len, filename, strlen(filename));
            strcat(serialised_str, "\n");
            current_len += (name_len + 1);
        }
        if (current_len != 0)
            if (!sendFilenamesToClient(client_fd, serialised_len, serialised_str)) {
                printf("failed!\n");
                return 0;
            }
        free(serialised_str);
        closedir(dir);
        if (!notifyFullFileSent(client_fd, 'G')) {
            printf("Server: error whilst trying to notify client that all filenames sent\n");
            return 0;
        }
        return 1;
    }
    else {
        closedir(dir);
        printf("Server: error whilst trying to open storage directory to read filenames\n");
        return 0;
    }
}
