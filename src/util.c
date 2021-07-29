#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "util.h"

void* getAddress(struct sockaddr* socket_addr) {
    if (socket_addr->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)socket_addr)->sin_addr);
    }
    return &(((struct sockaddr_in6*)socket_addr)->sin6_addr);
}

int sendAll(uint8_t fd, void* buffer, uint8_t* len) {
    uint8_t total = 0;
    uint8_t bytes_left = *len;
    uint8_t bytes_sent;
    while (total < *len) {
        printf("sendAll iteration: len %d, max %d\n", total, *len);
        bytes_sent = send(fd, buffer + total, bytes_left, 0);
        //printf("BYTES SENT: %d\n", bytes_sent);
        if (bytes_sent == -1) break;
        total += bytes_sent;
        bytes_left -= bytes_sent;
    }
    *len = total;
    return bytes_sent == -1 ? -1 : 0;
}

int recvAll(uint8_t fd, struct FileProtocolPacket** req) {
    uint8_t bytes_recv;
    const uint8_t header_len = 2;
    void* header_buffer = malloc(header_len);
    memset(header_buffer, 0, header_len);
    int len = recv(fd, header_buffer, 2, 0);
    if (len == 0 || len == -1) {
        printf("Connection (%d) failed on receiving, received: %d\n", fd, len);
        return 0;
    }
    printf("BYTES RECEIVED: %d\n", len);
    uint8_t packet_length = *((uint8_t*)header_buffer);
    if (!(len < packet_length)) {
        *req = (struct FileProtocolPacket*)header_buffer;
        return 1;
    }
    void* buffer = malloc(packet_length);
    memset(buffer, 0, packet_length);
    memcpy(buffer, header_buffer, len);
    while (len < packet_length) {
        printf("recvAll iteration: len %d, max %d\n", len, packet_length);
        bytes_recv = recv(fd, buffer + len, packet_length - len, 0);
        if (bytes_recv == -1) {
            printf("server: recvAll failed\n");
            return 0;
        }
        else if (bytes_recv == 0) {
            printf("server: socket %d hung up\n", fd);
            return 0;
        }
        len += bytes_recv;
    }
    *req = (struct FileProtocolPacket*)buffer;
    return 1;
}            

void constructPacket(uint8_t len, char* tag, void* data, void** packet) {
    const void* pktlen = &len;
    memcpy((*packet), (void*)&len, 1);
    memcpy(((*packet) + 1), (void*)tag, 1);
    memcpy(((*packet) + 2), data, (len - 2));
}

int uploadFile(uint8_t fd, char* filename) {
    FILE* file_ptr;
    file_ptr = fopen(filename, "r");
    char tag = 'U';
    char data[PKT_DATA_SIZE] = {0};
    printf("Inside upload file\n");
    while (fgets(data, PKT_DATA_SIZE, file_ptr) != NULL) {
        if (!strcmp(data, "\n")) {
            //bzero(data, PKT_DATA_SIZE);
            data[1] = '\0';
            //data[1] = '\n'; 
        }
        printf("line (%ld) from file: %s\n", strlen(data), data);
        uint8_t data_len = strlen(data) + 2;
        void* file_packet = malloc(strlen(data) + 2);
        constructPacket(data_len, &tag, data, &file_packet);
        if (sendAll(fd, file_packet, &data_len) == -1) {
            printf("Upload File: failed to send out packet");
            return 0;
        }
        bzero(data, PKT_DATA_SIZE);
        free(file_packet);
    }
    fclose(file_ptr);
    printf("data: %s\n", data);
    return 1;
}

int downloadFile(uint8_t fd, char* filename) {
    FILE* file_ptr = fopen(filename, "w");
    for (;;) {
        struct FileProtocolPacket* file_pkt;
        if (!recvAll(fd, &file_pkt)) {
            printf("Error: receiving packets failed whilst downloading file\n");
            return 0;
        }
        fprintf(file_ptr, "%s", file_pkt->filename);
        uint8_t len = strlen(file_pkt->filename);
        printf("line from file: %s\n", file_pkt->filename);
        if (file_pkt->filename[len] == '\0') {
        }
        free(file_pkt);
    }
    fclose(file_ptr);
}   