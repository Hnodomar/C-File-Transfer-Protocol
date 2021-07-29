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
    void* buffer = malloc(256);
    uint8_t len = recv(fd, buffer, 255, 0);
    if (len == 0 || len == -1) {
        printf("server: socket %d hung up\n", fd);
        return 0;
    }
    printf("BYTES RECEIVED: %d\n", len);
    uint8_t packet_length = *((uint8_t*)buffer);
    while (len < packet_length) {
        printf("recvAll iteration: len %d, max %d\n", len, packet_length);
        bytes_recv = recv(fd, buffer + len, 256 - len, 0);
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
        printf("line from file: %s\n", data);
        void* file_packet = malloc(strlen(data) + 2);
        uint8_t pkt_len = strlen(data) + 2;
        constructPacket(pkt_len, &tag, data, &file_packet);
        if (sendAll(fd, file_packet, &pkt_len) == -1) {
            printf("Upload File: failed to send out packet");
            return 0;
        }
        bzero(data, PKT_DATA_SIZE);
        free(file_packet);
    }
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
            printf("File downloaded successfully\n");
            return 1;
        }
        free(file_pkt);
    }
}   