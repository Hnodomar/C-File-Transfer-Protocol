#include <sys/socket.h>
#include <netinet/in.h>

#include "util.h"

void* getAddress(struct sockaddr* socket_addr) {
    if (socket_addr->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)socket_addr)->sin_addr);
    }
    return &(((struct sockaddr_in6*)socket_addr)->sin6_addr);
}
