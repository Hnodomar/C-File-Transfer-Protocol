struct FileProtocolPacket {
    uint8_t length;
    char tag;
    char filename[254];
};

extern void* getAddress(struct sockaddr* socket_addr);
extern int sendAll(uint8_t fd, void* buffer, uint8_t* len);
extern int recvAll(uint8_t fd, struct FileProtocolPacket** req);
extern void constructPacket(uint8_t len, char* tag, void* data, void** packet);
extern void uploadFile(uint8_t fd);