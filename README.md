# File Transfer in C

This project interacts directly with the Berkeley Sockets API to allow clients to upload/download files to/from a server over TCP, as well as retrieve a list of files currently stored on the server.

## Code Features

* A basic application protocol has been designed in order to allow the server and client to interpret packets correctly. Packets have a maximum length of 255 bytes. The first byte specifies the length of the packet (mandatory), the second byte the packet type (upload, download or getfiles, also mandatory) and the optional last 253 bytes may contain the data being transferred.

* It implements the classic client-server model of accepting a client's connection, then forking a child process to handle the request.

* Partial sends and receives are accounted for by wrapping send() and recv() system calls in sendAll() and recvAll() utility functions.

* A custom application protocol ensures that both client and server know whether there are more bytes to be read for a particular transfer of data.

* The code has full unit and integration test-suites.

## Dependencies

* Check - C Unit Testing Framework
* pkgconfig - Used for linking Check to build the unit tests
* Python3 - Used to implement integration testing
