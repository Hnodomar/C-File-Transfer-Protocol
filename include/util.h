#ifndef UTIL_H
#define UTIL_H

#define PKT_DATA_SIZE 253
#define HEADER_LEN 2
#define PORT "9034"

struct FileProtocolPacket {
    uint8_t length;
    char tag;
    char data[PKT_DATA_SIZE];
};

extern void* getAddress(struct sockaddr* socket_addr);
extern int sendAll(uint8_t fd, void* buffer, uint8_t* len);
extern int recvAll(uint8_t fd, struct FileProtocolPacket** req);
extern void constructPacket(uint8_t len, char* tag, void* data, void** packet);
extern int uploadFile(uint8_t fd, char* filename);
extern int downloadFile(uint8_t fd, char* filename);
extern int notifyFullFileSent(uint8_t fd, char tag);
extern int setupSocket(int setup_listener, const char* hostname);

#endif
