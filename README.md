# File Transfer Protocol in C

This project, written in C, interacts directly with the Berkeley sockets API to allow clients to upload/download files to/from a server, as well as retrieve a list of files currently stored on the server.

A basic custom application protocol has been designed in order to allow the server and client to talk and transfer files effectively.

## Code Features

The server implements a classic model: a client connection is accepted and then a process is forked to handle this connection. This way, a single clients connection doesn't block any other clients connection. This permits multiple the server to perform multiple uploads/downloads at a time.

Partial sends and receives are accounted for by wrapping send() and recv() system calls in sendAll() and recvAll() utility functions.

A custom application protocol ensures that both client and server know whether there are more bytes to be read for a particular transfer of data.

The code has full unit and integration test-suites.
