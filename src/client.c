//
// Created by Георгий Имешкенов on 11.12.2023.
//
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <printf.h>
#include <string.h>
#include <stdlib.h>
#include <SDL3/SDL.h>

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

    calloc_safe(
            struct sockaddr_in *, server_addr, 1, sizeof(struct sockaddr_in));
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

    malloc_safe(socklen_t *, server_addr_size, sizeof(struct sockaddr_in));
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

    calloc_safe(
            char *, buf, 1, MAXNETWORKBUFFSIZE);

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
create_client(const struct server_info_s *server_params, long buff_size, unsigned char *buff) {
    int *sock;
    socklen_t *server_addr_size;
    struct sockaddr_in *server_addr;
    struct client_s *client;

    sock = (int *) malloc(sizeof(int));

    *sock = socket(server_params->sock_fam, server_params->sock_type, 0);
    if (*sock == -1) {
        puts("error while creating socket");
        perror("socket");
        return NULL;
    }
    puts("socket inited");

    calloc_safe(
            struct sockaddr_in *, server_addr, 1, sizeof(struct sockaddr_in));

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

    printf("allocating mem for client struct...");
    malloc_safe(socklen_t *, server_addr_size, sizeof(struct sockaddr_in));
    *server_addr_size = sizeof(struct sockaddr_in);
    malloc_safe(
            struct client_s *, client, client_size)
    puts("done");
    puts("allocating mem for client buffer wrapper...");
    malloc_safe(
            struct buffer_s *, client->buffer, sizeof(struct buffer_s));
    client->buffer->max_len = buff_size;
    puts("done");

    puts("allocating mem for client buf...");
    if (buff == NULL) {
        puts("external buffer not provided, creating a new one...");
        calloc_safe(
                unsigned char *, client->buffer->data, client->buffer->max_len, sizeof(char))
    } else {
        puts("external buffer provided, using it...");
        calloc_safe(
                unsigned char *, buff, client->buffer->max_len, sizeof(char))
        client->buffer->data = buff;
    }
    puts("done");

    client->socket = sock;
    client->host_addr = server_addr;
    client->host_addr_len = server_addr_size;

    return client;
}

int
send_message(const struct client_s *client, unsigned char *buf) {
    int sent_bytes;
    sent_bytes = (int) sendto(
            *client->socket,
            buf,
//            NETWORK_BUFFER_OFFSET + 4 + 4 + strnlen((char *) (buf + 4 + 4 + NETWORK_BUFFER_OFFSET), MAXNETWORKBUFFSIZE),
            client->buffer->fill,
            MSG_NOSIGNAL,
            (struct sockaddr *) client->host_addr,
            *client->host_addr_len
    );
    int recv_bytes;
//    recv_bytes = recvfrom(
//            sock_client,
//            buff,
//            MAXNETWORKBUFFSIZE,
//            0,
//            client_addr,
//            client_addr_size
//    );
    return sent_bytes;
}

int
send_message_with_client_only(const struct client_s *client) {
    return send_message(client, client->buffer->data);
}

int
send_message_and_flush(const struct client_s *client, unsigned char *buf) {
    int sent_bytes;
    sent_bytes = send_message(client, buf);
    memset(buf, 0, sent_bytes + 1);
    return sent_bytes;
}


int
send_msg_internal(int *conn, int sock, char *buf, const struct sockaddr *server_addr, socklen_t *server_addr_size) {
    player_obj *player = (player_obj *) (malloc(sizeof(struct player_obj_s)));
    player->pos->x = 0;
    player->pos->y = 0;

    return -2;
}