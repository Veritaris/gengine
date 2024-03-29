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
#include <arpa/inet.h>
#include <netdb.h>

#define BACKLOG 16

void
serve_unix(const struct server_info_s *server_params) {
    int sock, sock_client;
    socklen_t peer_addr_size, addr_size;
    struct sockaddr_un *server;
    struct sockaddr_un peer_addr;
    char *buf;

    unlink(server_params->socket_path);

    sock = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sock == -1) {
        printf("unable to create socket: %d\n", errno);
        return;
    }

    server = calloc(1, sizeof(struct sockaddr_un));
    server->sun_family = AF_UNIX;
    strncpy(server->sun_path, server_params->socket_path, sizeof(server->sun_path));

    addr_size = sizeof(server->sun_family) + strlen(server->sun_path) + 1;
    printf("binding socket socket '%d' to address '%s'\n", sock, server->sun_path);

    if (bind(sock, (struct sockaddr *) server, addr_size) == -1) {
        printf("unable to bind socket '%s'\n", server->sun_path);
        perror("bind");
        return;
    }

    printf("server started\n");
    printf("listening started\n");

    if (listen(sock, BACKLOG) == -1) {
        printf("unable to start listening: %d\n", errno);
        perror("listen");
        return;
    }

    printf("accepting connection...\n");

    peer_addr_size = sizeof(struct sockaddr);
    sock_client = accept(sock, (struct sockaddr *) &peer_addr, &peer_addr_size);

    if (sock_client == -1) {
        printf("unable to accept connection: %s\n", server_params->socket_path);
        perror("accept");
        return;
    }
    printf("client '%s' connected\n", peer_addr.sun_path);

    buf = calloc(1, MAXNETWORKBUFFSIZE);

    handle_client(sock_client, buf, (struct sockaddr *) &peer_addr, &peer_addr_size);

    free(buf);

    if (close(sock) == -1) {
        printf("unable to close sock\n");
        return;
    }

    if (unlink(server_params->socket_path) == -1) {
        printf("unable to unlink socket: '%s'", server_params->socket_path);
        return;
    }
    printf("server exited\n");
}

void
serve_inet(const struct server_info_s *server_params) {
    int host_sock, peer_sock;
    socklen_t *peer_addr_size;
    struct sockaddr_in *host_addr, *peer_addr;
    char *buf;
    int yes = 1;

    printf("starting inet server at '%s:%d'\n", server_params->address, server_params->port);

    host_sock = socket(server_params->sock_fam, server_params->sock_type, 0);

    if (host_sock == -1) {
        allocwarn("server socket");
        return;
    }

    if (setsockopt(
            host_sock,
            SOL_SOCKET,
            SO_REUSEADDR,
            &yes,
            sizeof(int)
    ) == -1) {
        perror("setsockopt");
    }

    host_addr = calloc(1, sizeof(struct sockaddr_in));
    if (host_addr == NULL) {
        allocwarn("server address");
        exit(-1);
    }
    host_addr->sin_family = server_params->sock_fam;
    host_addr->sin_port = htons(server_params->port);

    switch (server_params->sock_fam) {
        case AF_INET:
            inet_pton(AF_INET, server_params->address, &(host_addr->sin_addr));
            break;
        case AF_INET6:
            inet_pton(AF_INET6, server_params->address, &(host_addr->sin_addr));
            break;
        default:
            printf("unknown sock_fam: %d\n", server_params->sock_fam);
            exit(-1);
    }

    printf(
            "binding socket '%d' with address '%s:%d'\n",
            host_sock,
            inet_ntoa(host_addr->sin_addr),
            ntohs(host_addr->sin_port)
    );

    if (BIND(host_sock, host_addr) == -1) {
        printf("unable to bind socket '%s:%d'\n", inet_ntoa(host_addr->sin_addr), ntohs(host_addr->sin_port));
        perror("bind");
//        we want to try to unbind socket on error
        close(host_sock);
        return;
    }

    peer_addr_size = malloc(sizeof(struct sockaddr));
    peer_addr = malloc(sizeof(struct sockaddr_in));

    if (peer_addr_size == NULL || peer_addr == NULL) {
        allocwarn("client addr size and client address");
        exit(-1);
    }
    *peer_addr_size = sizeof(struct sockaddr);

    if (IS_STREAM) {
        printf("listening started\n");

        if (listen(host_sock, BACKLOG) == -1) {
            printf("unable to start listening at '%s:%d', errno=%d\n", inet_ntoa(host_addr->sin_addr),
                   ntohs(host_addr->sin_port), errno);
            return;
        }

        printf("Waiting for connection...\n");
        peer_sock = accept(host_sock, (struct sockaddr *) peer_addr, peer_addr_size);

        if (peer_sock == -1) {
            printf("unable to accept connection\n");
            return;
        }

        printf("client '%s:%d' connected\n", inet_ntoa(peer_addr->sin_addr), ntohl(peer_addr->sin_port));
    }

    buf = calloc(1, MAXNETWORKBUFFSIZE);
    if (buf == NULL) {
        allocwarn("data buffer on server side");
        exit(-1);
    }

    if (IS_STREAM) {
        handle_client(peer_sock, buf, (struct sockaddr *) peer_addr, peer_addr_size);
    } else {
        handle_client(host_sock, buf, (struct sockaddr *) peer_addr, peer_addr_size);
    }

    free(buf);
    free(peer_addr);
    free(peer_addr_size);
    free(host_addr);

    if (close(host_sock) == -1) {
        printf("unable to close host_sock\n");
        return;
    }

    char *prefix = malloc(INET6_ADDRSTRLEN);
    in_addr_t *nclient_addr = &((struct sockaddr_in *) peer_addr)->sin_addr.s_addr;
    printf("%s executed stopserver\n", inet_ntop(AF_INET, (const void *) nclient_addr, prefix, *peer_addr_size));
    free(prefix);
}

void
handle_client(int sock_client, char *buff, struct sockaddr *client_addr, socklen_t *client_addr_size) {
    ssize_t recv_bytes;
    char *result;
    char *prefix;
    while (1) {
        recv_bytes = recvfrom(
                sock_client,
                buff,
                MAXNETWORKBUFFSIZE,
                0,
                client_addr,
                client_addr_size
        );

        if (recv_bytes > 0) {
            if (recv_bytes < MAXNETWORKBUFFSIZE) {
                buff[++recv_bytes] = '\0';
            }
            result = malloc(recv_bytes);
            memcpy(result, buff, recv_bytes);

            malloc_save(char *, prefix, NETWORK_BUFFER_OFFSET);

            in_addr_t *nclient_addr = &((struct sockaddr_in *) client_addr)->sin_addr.s_addr;
            printf(
                    "[%s] ",
                    inet_ntop(AF_INET, (const void *) nclient_addr, prefix, *client_addr_size)
            );
            printf("%s\n", result + NETWORK_BUFFER_OFFSET);

            if (strcmp(result, "stopserver") == 0) {
                printf("received 'exit' command, stopping the server\n");
                break;
            }

            if (strcmp(result, "disconnect") == 0) {
                printf("client [%s] disconnected\n", inet_ntop(AF_INET, (const void *) nclient_addr, prefix, *client_addr_size));
            }


            memset(buff, 0, recv_bytes);
            memset(result, 0, recv_bytes);
        }

        usleep((unsigned int) 1e4);
    }
    free(result);
    free(prefix);
}

void
sigint_handler(int signum) {
    unlink(SOCK_PATH);
    exit(0);
}

int
main(int argc, char **argv) {
    signal(SIGINT, sigint_handler);
    struct server_info_s *server_params;
    malloc_save(struct server_info_s *, server_params, server_info_size);

    if (argc == 1) {
        printf("usage: %s <unix|local> [descriptor path] or <inet> [host] [port]\n", argv[0]);
        return -1;
    }

    fill_server_info(server_params, argc, argv);

    switch (server_params->type) {
        case UNIX:
            printf("running unix client\n");
            serve_unix(server_params);
            break;
        case INET:
            printf("running inet client\n");
            serve_inet(server_params);
            break;
        case ERROR:
            printf("unknown server type\n");
            printf("usage: %s <unix|local> [descriptor path] or <inet> [host] [port]\n", argv[0]);
            break;
    }
    free(server_params);
    return 0;
}
