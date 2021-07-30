#define PKT_DATA_SIZE 254
#define HEADER_LEN 2

struct FileProtocolPacket {
    uint8_t length;
    char tag;
    char filename[PKT_DATA_SIZE];
};

extern void* getAddress(struct sockaddr* socket_addr);
extern int sendAll(uint8_t fd, void* buffer, uint8_t* len);
extern int recvAll(uint8_t fd, struct FileProtocolPacket** req);
extern void constructPacket(uint8_t len, char* tag, void* data, void** packet);
extern int uploadFile(uint8_t fd, char* filename);
extern int downloadFile(uint8_t fd, char* filename);
extern int notifyFullFileSent(uint8_t fd);