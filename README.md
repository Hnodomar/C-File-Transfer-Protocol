# UDP File Transfer

Written in C, this project makes use of the Berkeley sockets API to allow clients to upload files to a server, fetch a list of files already on the server and download files from the server.

Clients connect via TCP to access an interface. Through this interface, they may either download or upload files over UDP.
