#ifndef FILENAMES_H
#define FILENAMES_H

int serialiseAndSendFilenames(uint8_t client_fd);
int sendFilenamesToClient(uint8_t client_fd, const uint8_t serialised_len, char* serialised_str);

#endif
