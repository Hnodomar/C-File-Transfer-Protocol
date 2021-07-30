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
        bytes_sent = send(fd, buffer + total, bytes_left, 0);
        if (bytes_sent == -1) break;
        total += bytes_sent;
        bytes_left -= bytes_sent;
    }
    *len = total;
    return bytes_sent == -1 ? -1 : 0;
}

int recvAll(uint8_t fd, struct FileProtocolPacket** req) {
    uint8_t bytes_recv;
    void* header_buffer = malloc(HEADER_LEN);
    memset(header_buffer, 0, HEADER_LEN);
    int len = recv(fd, header_buffer, HEADER_LEN, 0);
    if (len == 0 || len == -1) {
        printf("Connection (%d) failed on receiving, received: %d\n", fd, len);
        return 0;
    }
    uint8_t packet_length = *((uint8_t*)header_buffer);
    if (!(len < packet_length)) {
        *req = (struct FileProtocolPacket*)header_buffer;
        return 1;
    }
    void* buffer = malloc(packet_length + 1);
    memset(buffer, 0, packet_length);
    memcpy(buffer, header_buffer, len);
    while (len < packet_length) {
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
    *((char*)buffer + (packet_length)) = '\0';
    *req = (struct FileProtocolPacket*)buffer;
    buffer = NULL;
    return 1;
}            

void constructPacket(uint8_t len, char* tag, void* data, void** packet) {
    memcpy((*packet), (void*)&len, 1);
    memcpy(((*packet) + 1), (void*)tag, 1);
    memcpy(((*packet) + HEADER_LEN), data, (len - HEADER_LEN));
}

int notifyFullFileSent(uint8_t fd, char tag) {
    const char* data = "END";
    uint8_t data_len = strlen(data) + HEADER_LEN;
    void* packet = malloc(data_len);
    constructPacket(data_len, &tag, (void*)data, &packet);
    if (sendAll(fd, packet, &data_len) == -1) {
        printf("notifyFullFileSent: failed to notify remote that file was sent successfully\n");
        return 0;
    }
    return 1;
}

int uploadFile(uint8_t fd, char* filename) {
    FILE* file_ptr;
    file_ptr = fopen(filename, "r");
    char tag = 'U';
    char data[PKT_DATA_SIZE] = {0};
    while (fgets(data, PKT_DATA_SIZE, file_ptr) != NULL) {
        uint8_t len = strlen(data);
        uint8_t data_len = strlen(data) + HEADER_LEN;
        void* file_packet = malloc(data_len);
        constructPacket(data_len, &tag, data, &file_packet);
        if (sendAll(fd, file_packet, &data_len) == -1) {
            printf("Upload File: failed to send out packet");
            return 0;
        }
        bzero(data, PKT_DATA_SIZE);
        free(file_packet);
        file_packet = NULL;
    }
    fclose(file_ptr);
    printf("File %s successfully sent to socket %d\n", filename, fd);
    if (!notifyFullFileSent(fd, 'U'))
        return 0;
    return 1;
}

int downloadFile(uint8_t fd, char* filename) {
    FILE* file_ptr = fopen(filename, "w");
    for (;;) {
        struct FileProtocolPacket* file_pkt;
        if (!recvAll(fd, &file_pkt)) {
            printf("Error: receiving packets failed whilst downloading file\n");
            fclose(file_ptr);
            return 0;
        }
        if (!strcmp(file_pkt->filename, "END")) {
            printf("File %s successfully downloaded!\n", filename);
            fclose(file_ptr);
            return 1;
        }
        fprintf(file_ptr, "%s", file_pkt->filename);
        free(file_pkt);
        file_pkt = NULL;
    }
    fclose(file_ptr);
}   
