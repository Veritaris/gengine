//
// Created by Георгий Имешкенов on 11.12.2023.
//
#include <sys/socket.h>
#include <sys/un.h>
#include <printf.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "network.h"

int
client_unix(const struct server_info_s *server_params) {
    int sock;
    int conn;
    char *buf;
    socklen_t *server_addr_size;
    struct sockaddr_un *server_addr;
    int sent_bytes;

    sock = socket(server_params->sock_fam, server_params->sock_type, 0);

    if (sock == -1) {
        printf("error while creating socket\n");
        perror("socket");
        return -1;
    }

    server_addr = (struct sockaddr_un *) malloc(sizeof(struct sockaddr_un));
    if (server_addr == NULL) {
        allocwarn("server_addr address");
        exit(-1);
    }

    server_addr->sun_family = server_params->sock_fam;
    strncpy(server_addr->sun_path, server_params->socket_path, sizeof(server_addr->sun_path));

    server_addr_size = (socklen_t *) malloc(sizeof(socklen_t));
    if (server_addr_size == NULL) {
        allocwarn("server_addr address size");
        exit(-1);
    }
    *server_addr_size = sizeof(server_addr->sun_family) + strlen(server_addr->sun_path) + 1;

    conn = connect(
            sock,
            (const struct sockaddr *) server_addr,
            *server_addr_size
    );

    if (conn == -1) {
        printf("error while connecting to socket\n");
        perror("connect");
        return -1;
    }

    printf("connected to server_addr\n");

    buf = (char *) calloc(1, MAXNETWORKBUFFSIZE);
    if (buf == NULL) {
        allocwarn("data buffer on client side");
        exit(-1);
    }

    while (1) {
        sent_bytes = send_msg_internal(&conn, sock, buf, (const struct sockaddr *) server_addr, server_addr_size);
        if (sent_bytes == -2) {
            break;
        }
    }

    free(buf);
    free(server_addr_size);
    free(server_addr);

    return 0;
}

int
client_inet(const struct server_info_s *server_params) {
    int sock;
    int conn;
    char *buf;
    socklen_t *server_addr_size;
    struct sockaddr_in *server_addr;
    int sent_bytes;
    int udp_use_connect = 0;

    sock = socket(server_params->sock_fam, server_params->sock_type, 0);
    if (sock == -1) {
        printf("error while creating socket\n");
        perror("socket");
        return -1;
    }

    calloc_save(struct sockaddr_in *, server_addr, 1, sizeof(struct sockaddr_in));
    server_addr->sin_family = server_params->sock_fam;
    server_addr->sin_port = htons(server_params->port);

    switch (server_params->sock_fam) {
        case AF_INET:
            inet_pton(AF_INET, server_params->address, &(server_addr->sin_addr));
            break;
        case AF_INET6:
            inet_pton(AF_INET6, server_params->address, &(server_addr->sin_addr));
            break;
        default:
            printf("unknown sock_fam: %d\n", server_params->sock_fam);
            exit(-1);
    }

    malloc_save(socklen_t *, server_addr_size, sizeof(struct sockaddr_in));
    *server_addr_size = sizeof(struct sockaddr_in);

    if (IS_STREAM | udp_use_connect) {
        printf("connecting to server '%s:%d'...\n", inet_ntoa(server_addr->sin_addr), ntohs(server_addr->sin_port));
        conn = connect(
                sock,
                (const struct sockaddr *) server_addr,
                sizeof(struct sockaddr_in)
        );

        if (conn == -1) {
            printf("error while connecting to server\n");
            perror("connect");
            return -1;
        }

        printf("connected to '%s:%d'\n", inet_ntoa(server_addr->sin_addr), ntohs(server_addr->sin_port));
    }

    calloc_save(char *, buf, 1, MAXNETWORKBUFFSIZE);

    while (1) {
        sent_bytes = send_msg_internal(&conn, sock, buf, (const struct sockaddr *) server_addr, server_addr_size);
        if (sent_bytes == -2) {
            break;
        }
    }
    free(buf);
    free(server_addr_size);
    free(server_addr);
    return 0;
}

struct client_s *
create_client(const struct server_info_s *server_params) {
    int *sock;
    socklen_t *server_addr_size;
    struct sockaddr_in *server_addr;
    struct client_s *client;

    sock = (int *) malloc(sizeof(int));

    *sock = socket(server_params->sock_fam, server_params->sock_type, 0);
    if (*sock == -1) {
        printf("error while creating socket\n");
        perror("socket");
        return NULL;
    }

    calloc_save(struct sockaddr_in *, server_addr, 1, sizeof(struct sockaddr_in));

    server_addr->sin_family = server_params->sock_fam;
    server_addr->sin_port = htons(server_params->port);

    switch (server_params->sock_fam) {
        case AF_INET:
            inet_pton(AF_INET, server_params->address, &(server_addr->sin_addr));
            break;
        case AF_INET6:
            inet_pton(AF_INET6, server_params->address, &(server_addr->sin_addr));
            break;
        default:
            printf("unknown sock_fam: %d\n", server_params->sock_fam);
            exit(-1);
    }

    malloc_save(socklen_t *, server_addr_size, sizeof(struct sockaddr_in));
    *server_addr_size = sizeof(struct sockaddr_in);
    malloc_save(struct client_s *, client, client_size);

    client->socket = sock;
    client->host_addr = server_addr;
    client->host_addr_len = server_addr_size;

    return client;
}

int
send_message(const struct client_s *client, char *buf) {
    int sent_bytes;
    sent_bytes = (int) sendto(
            *client->socket,
            buf,
            NETWORK_BUFFER_OFFSET + strnlen(buf + NETWORK_BUFFER_OFFSET, MAXNETWORKBUFFSIZE),
            MSG_NOSIGNAL,
            (struct sockaddr *) client->host_addr,
            *client->host_addr_len
    );
    return sent_bytes;
}

int
send_message_and_flush(const struct client_s *client, char *buf) {
    int sent_bytes;
    sent_bytes = send_message(client, buf);
    memset(buf, 0, sent_bytes + 1);
    return sent_bytes;
}

int
send_msg_internal(int *conn, int sock, char *buf, const struct sockaddr *server_addr, socklen_t *server_addr_size) {
//    TODO
    return -2;
}

