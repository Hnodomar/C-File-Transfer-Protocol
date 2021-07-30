int readFilenames(char*** names, size_t names_size, uint16_t* files_str_len);
int serialiseFilenames(char*** names, char** names_serialised, int max_size, int num_files);
int getFilenames(uint8_t sender_fd);