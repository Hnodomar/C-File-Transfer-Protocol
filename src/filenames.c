#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdint.h>

#include "filenames.h"

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
