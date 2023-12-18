//
// Created by Георгий Имешкенов on 11.12.2023.
//
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <string.h>
#include <printf.h>
#include <sys/errno.h>
#include <unistd.h>
#include "network.h"
#include <signal.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define BACKLOG 16

void
serve_unix(const char *socket_path) {
    int sock, sock_client;
    socklen_t peer_addr_size, addr_size;
    struct sockaddr_un *server;
    struct sockaddr_un peer_addr;
    char *buf;

    unlink(socket_path);

    sock = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sock == -1) {
        printf("unable to create socket: %d\n", errno);
        return;
    }

    server = calloc(1, sizeof(struct sockaddr_un));
    server->sun_family = AF_UNIX;
    strncpy(server->sun_path, socket_path, sizeof(server->sun_path));

    addr_size = sizeof(server->sun_family) + strlen(server->sun_path) + 1;
    printf("binding socket socket '%d' to address '%s'\n", sock, server->sun_path);

    if (bind(sock, (struct sockaddr *) server, addr_size) == -1) {
        printf("unable to bind socket '%s'\n", server->sun_path);
        handle_socket_bind(sock);
        return;
    }

    printf("server started\n");
    printf("listening started\n");

    if (listen(sock, BACKLOG) == -1) {
        printf("unable to start listening: %d\n", errno);
        return;
    }

    printf("accepting connection...\n");

    peer_addr_size = sizeof(struct sockaddr);
    sock_client = accept(sock, (struct sockaddr *) &peer_addr, &peer_addr_size);

    if (sock_client == -1) {
        printf("unable to accept connection: %s\n", socket_path);
        return;
    }
    printf("client '%s' connected\n", peer_addr.sun_path);

    buf = calloc(1, MAXDATABUFFLEN);

    start_messaging(sock_client, buf, (struct sockaddr *) &peer_addr, &peer_addr_size);

    free(buf);

    if (close(sock) == -1) {
        printf("unable to close sock\n");
        return;
    }

    if (unlink(socket_path) == -1) {
        printf("unable to unlink socket: '%s'", socket_path);
        return;
    }
    printf("server exited\n");
}

void
serve_inet(const char *host, int port) {
    int server_sock, sock_client;
    socklen_t *client_addr_size;
    struct sockaddr_in *server_addr, *client_addr;
    char *buf;

    printf("starting inet server at '%s:%d'\n", host, port);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (server_sock == -1) {
        allocwarn("server socket");
        return;
    }
//    we want to ensure that sock is closed; also we do no care about result
    close(server_sock);

    server_addr = calloc(1, sizeof(struct sockaddr_in));
    if (server_addr == NULL) {
        allocwarn("server address");
        exit(-1);
    }
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);
    inet_aton(host, &server_addr->sin_addr);

    printf("binding socket '%d' with address '%s:%d'\n", server_sock, inet_ntoa(server_addr->sin_addr),
           ntohs(server_addr->sin_port));

    if (bind(server_sock, (struct sockaddr *) server_addr, sizeof(struct sockaddr_in)) == -1) {
        printf("unable to bind socket '%s:%d'\n", inet_ntoa(server_addr->sin_addr), ntohs(server_addr->sin_port));
        handle_socket_bind(server_sock);
        return;
    }

    printf("listening started\n");

    if (listen(server_sock, BACKLOG) == -1) {
        printf("unable to start listening at '%s:%d', errno=%d\n", inet_ntoa(server_addr->sin_addr),
               ntohs(server_addr->sin_port), errno);
        return;
    }

    printf("Waiting for connection...\n");

    client_addr_size = malloc(sizeof(struct sockaddr));
    client_addr = malloc(sizeof(struct sockaddr_in));

    if (client_addr_size == NULL || client_addr == NULL) {
        allocwarn("client addr size and client address");
        exit(-1);
    }

    *client_addr_size = sizeof(struct sockaddr);
    sock_client = accept(server_sock, (struct sockaddr *) client_addr, client_addr_size);

    if (sock_client == -1) {
        printf("unable to accept connection\n");
        return;
    }
    printf("client '%s:%d' connected\n", inet_ntoa(client_addr->sin_addr), client_addr->sin_port);

    buf = calloc(1, MAXDATABUFFLEN);
    if (buf == NULL) {
        allocwarn("data buffer");
        exit(-1);
    }

    start_messaging(sock_client, buf, (struct sockaddr *) &client_addr, client_addr_size);

    free(buf);

    if (close(server_sock) == -1) {
        printf("unable to close server_sock\n");
        return;
    }

    printf("server_addr exited\n");
}

void
start_messaging(int sock_client, char *buff, struct sockaddr *client_addr, socklen_t *client_addr_size) {
    ssize_t recv_bytes;
    char *result;
    while (1) {
        recv_bytes = recvfrom(
                sock_client,
                buff,
                MAXDATABUFFLEN,
                0,
                client_addr,
                client_addr_size
        );

        if (recv_bytes > 0) {
            result = malloc(recv_bytes);
            memcpy(result, buff, recv_bytes);
            printf("%s\n", result);

            if (strcmp(result, "exit") == 0) {
                printf("received 'exit' command, stopping the server\n");
                break;
            }

            memset(buff, 0, recv_bytes);
            memset(result, 0, recv_bytes);
        }
        usleep(100);
    }
    free(result);
}

void
sigint_handler(int signum) {
    unlink(SOCK_PATH);
    exit(0);
}

int
main(int argc, char **argv) {
    signal(SIGINT, sigint_handler);
    enum ServerType server_type = ERROR;
    char *socket_path = SOCK_PATH;
    char *type;
    char *host = "127.0.0.1";
    int port = 10312;

    switch (argc) {
        case 1:
            break;
        case 2: {
            type = argv[1];
            if (strcmp(type, "unix") == 0 || strcmp(type, "local") == 0) {
                server_type = UNIX | LOCAL;
            } else if (strcmp(type, "inet") == 0) {
                server_type = INET;
            } else {
                server_type = ERROR;
            }
            break;
        }
        case 3: {
            type = argv[1];
            if (strcmp(type, "unix") == 0 || strcmp(type, "local") == 0) {
                server_type = UNIX | LOCAL;
                socket_path = argv[2];
            } else if (strcmp(type, "inet") == 0) {
                server_type = INET;
                host = argv[2];
            } else {
                server_type = ERROR;
            }
            break;
        }
        case 4: {
            type = argv[1];
            if (strcmp(type, "unix") == 0 || strcmp(type, "local") == 0) {
                server_type = UNIX | LOCAL;
                socket_path = argv[2];
            } else if (strcmp(type, "inet") == 0) {
                server_type = INET;
                host = argv[2];
                port = (int) strtol(argv[3], NULL, 10);
            } else {
                server_type = ERROR;
            }
            break;
        }
        default:
            type = argv[1];
            if (strcmp(type, "unix") == 0 || strcmp(type, "local") == 0) {
                server_type = UNIX | LOCAL;
                socket_path = argv[2];
            } else if (strcmp(type, "inet") == 0) {
                server_type = INET;
                host = argv[2];
                port = (int) strtol(argv[3], NULL, 10);
            } else {
                server_type = ERROR;
            }
            break;
    }

    switch (server_type) {
        case ERROR:
            printf("unknown server type\n");
            printf("usage: %s <unix|local> [descriptor path] or <inet> [host] [port]\n", argv[0]);
            break;
        case UNIX:
            serve_unix(socket_path);
            break;
        case INET:
            serve_inet(host, port);
            break;
    }
    return 0;
}