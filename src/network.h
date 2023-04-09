#pragma once

#include <poll.h>
#include <sys/socket.h>

#define SERVER_DEFAULT_PORT 1337
#define MAX_SERVER_CONN 20
#define SERVER_BACKLOG 20
#define BUFFER_SIZE 1024

typedef struct ClientConnection_t {
    int clientFd;
    struct sockaddr *addr;
    socklen_t addrLen;
} ClientConnection_t;

typedef struct Server_t {
    int serverFd;
    int port;
    ClientConnection_t clients[MAX_SERVER_CONN];
    struct pollfd pollFds[MAX_SERVER_CONN + 1];
} Server_t;

typedef void (*data_handler_t)(int clientFd, const char *data, int size, struct sockaddr *addr, socklen_t addrLen);

/**
 * Creates the server represented by the parameter server
 *
 * @param port The port to listen on
 *
 * @returns The struct representing the server, or NULL on error
 */
Server_t *createServer(int port);

/**
 * Destroy server
 */
void destroyServer(Server_t *server);

/**
 * Run the server.
 * Blocks and waits for some client requests to come in and then calls the callback function
 * with the data sent by the client.
 * Will also accept new clients if a new client is connecting to the server.
 */
void runServer(Server_t *server, data_handler_t onData);

int sendClientData(int clientFd, const char *data, int size);