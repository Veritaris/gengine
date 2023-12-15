//
// Created by Георгий Имешкенов on 11.12.2023.
//
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <printf.h>
#include <sys/errno.h>
#include <unistd.h>
#include "network.h"
#include <signal.h>
#include <stdlib.h>

#define BACKLOG 16

int
serve() {
    int sock, sock_client;
    socklen_t peer_addr_size, addr_size;
    struct sockaddr_un *server;
    struct sockaddr_un peer_addr;
    int *buff;
    ssize_t recv_bytes;
    char *result;

    unlink(SOCK_PATH);

    sock = socket(SOCK_FAM, SOCK_TYPE, 0);

    if (sock == -1) {
        printf("unable to create socket: %d\n", errno);
        return -1;
    }

    server = calloc(1, sizeof(struct sockaddr_un));
    server->sun_family = SOCK_FAM;
    strncpy(server->sun_path, SOCK_PATH, sizeof(server->sun_path));

    addr_size = sizeof(server->sun_family) + strlen(server->sun_path) + 1;
    printf("binding socket socket %d to address %s\n", sock, server->sun_path);

    if (bind(sock, (struct sockaddr *) server, addr_size) == -1) {
        printf("unable to bind socket '%s'\n", server->sun_path);
        handle_socket_bind(sock);
        return -1;
    }

    printf("server started\n");
    printf("listening started\n");

    if (listen(sock, BACKLOG) == -1) {
        printf("unable to start listening: %d\n", errno);
        return -1;
    }

    printf("accepting connection...\n");

    peer_addr_size = sizeof(struct sockaddr);
    sock_client = accept(sock, (struct sockaddr *) &peer_addr, &peer_addr_size);

    if (sock_client == -1) {
        printf("unable to accept connection: %s\n", SOCK_PATH);
        return -1;
    }
    printf("client '%s' connected\n", peer_addr.sun_path);

    buff = calloc(1, MAXDATABUFFLEN);
    while (1) {
        memset(buff, 0, MAXDATABUFFLEN);

        recv_bytes = recvfrom(
                sock_client,
                buff,
                MAXDATABUFFLEN,
                0,
                (struct sockaddr *) &peer_addr,
                (socklen_t *) &peer_addr_size
        );

        printf("received %zd bytes\n", recv_bytes);

        if (recv_bytes > 0) {
            result = malloc(recv_bytes * sizeof(int));
            memcpy(result, buff, recv_bytes);
            printf("%s\n", result);

        }
        memset(buff, 0, recv_bytes);
        memset(result, 0, recv_bytes);
        if (strcmp(result, "") == 0) {
            break;
        }
        usleep(100);
    }

    free(buff);
    free(result);

    if (close(sock) == -1) {
        printf("unable to close sock\n");
        return -1;
    }

    if (unlink(SOCK_PATH) == -1) {
        printf("unable to unlink socket: '%s'", SOCK_PATH);
        return -1;
    }
    printf("server exited\n");
    return 0;
}

void
sigint_handler(int signum) {
    unlink(SOCK_PATH);
    exit(0);
}

int
main() {
    signal(SIGINT, sigint_handler);
    return serve();
}