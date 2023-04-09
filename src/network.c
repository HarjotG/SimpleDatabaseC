#include "network.h"
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

Server_t *createServer(int port) {
    int socketFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (socketFd < 0) {
        printf("Error creating socket\n");
        return NULL;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(socketFd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        printf("Error binding socket %d\n", errno);
        close(socketFd);
        return NULL;
    }

    if (listen(socketFd, SERVER_BACKLOG) < 0) {
        printf("Error listening on socket\n");
        close(socketFd);
        return NULL;
    }
    Server_t *server = malloc(sizeof(Server_t));
    server->serverFd = socketFd;
    server->port = port;
    for (int i = 0; i < MAX_SERVER_CONN; i++) {
        server->clients[i].clientFd = -1;
        server->clients[i].addr = NULL;
        server->clients[i].addrLen = 0;
    }
    memset(server->pollFds, -1, (MAX_SERVER_CONN + 1) * sizeof(struct pollfd));
    return server;
}

void destroyServer(Server_t *server) {
    if (server != NULL) {
        close(server->serverFd);
        for (int i = 0; i < MAX_SERVER_CONN; i++) {
            ClientConnection_t client = server->clients[i];
            if (client.clientFd != -1) {
                free(client.addr);
                close(client.clientFd);
            }
        }
        free(server);
    }
}

int acceptClientConnections(Server_t *server) {
    int numAccept = 0;
    while (1) {
        int clientIdx = -1;
        for (int i = 0; i < MAX_SERVER_CONN; i++) {
            if (server->clients[i].clientFd == -1) {
                clientIdx = i;
            }
        }
        if (clientIdx == -1) {
            // Max connections reached, accept and then close the socket
            int closeFd = accept(server->serverFd, NULL, NULL);
            if (closeFd == -1) {
                // accept will return -1 when there are no more clients to accept
                return numAccept;
            }
            close(closeFd);
            return -1;
        }
        server->clients[clientIdx].addr = malloc(sizeof(struct sockaddr_in));
        int clientFd = accept(server->serverFd, server->clients[clientIdx].addr, &server->clients[clientIdx].addrLen);
        if (clientFd == -1) {
            // accept will return -1 when there are no more clients to accept
            break;
        }
        server->clients[clientIdx].clientFd = clientFd;
        // also set up client in pollFds to listen for incoming data
        // offset clientIdx by 1 to account for the first slot being taken by the server socket
        server->pollFds[clientIdx + 1].fd = clientFd;
        server->pollFds[clientIdx + 1].events = POLLIN;
    }
    return numAccept;
}

void runServer(Server_t *server, data_handler_t onData) {
    // set up server socket to listen for new connections to the server
    server->pollFds[0].fd = server->serverFd;
    server->pollFds[0].events = POLLIN;

    while (1) {
        int numReady = poll(server->pollFds, MAX_SERVER_CONN + 1, -1);
        if (numReady == -1) {
            printf("Error in poll()\n");
            break;
        }
        // check for new client connections
        if (server->pollFds[0].revents & POLLIN) {
            if (acceptClientConnections(server) < 0) {
                // Reached max client connections
                printf("Max connections reached\n");
            }
        }
        for (int i = 0; i < MAX_SERVER_CONN; i++) {
            if (server->pollFds[i + 1].revents & POLLIN) {
                // data to read from client
                char buffer[BUFFER_SIZE];
                int size = recv(server->clients[i].clientFd, buffer, BUFFER_SIZE, 0);
                if (size <= 0) {
                    // The client disconnected
                    close(server->clients[i].clientFd);
                    server->clients[i].clientFd = -1;
                    server->pollFds[i + 1].fd = -1;
                    free(server->clients[i].addr);
                } else {
                    onData(server->clients[i].clientFd, buffer, size, server->clients[i].addr, server->clients[i].addrLen);
                }
            }
        }
    }
    destroyServer(server);
}

int sendClientData(int clientFd, const char *data, int size) {
    if (send(clientFd, data, size, 0) < 0) {
        return -1;
    }
    return 0;
}