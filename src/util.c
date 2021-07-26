#include <sys/socket.h>
#include <netinet/in.h>

#include "util.h"

void* getAddress(struct sockaddr* socket_addr) {
    if (socket_addr->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)socket_addr)->sin_addr);
    }
    return &(((struct sockaddr_in6*)socket_addr)->sin6_addr);
}

uint8_t sendAll(uint8_t fd, void* buffer, uint8_t* len) {
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