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
client_unix(const struct server_info *server_params) {
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

    server_addr = malloc(sizeof(struct sockaddr_un));
    if (server_addr == NULL) {
        allocwarn("server_addr address");
        exit(-1);
    }

    server_addr->sun_family = server_params->sock_fam;
    strncpy(server_addr->sun_path, server_params->socket_path, sizeof(server_addr->sun_path));

    server_addr_size = malloc(sizeof(socklen_t));
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

    buf = calloc(1, MAXDATABUFFLEN);
    if (buf == NULL) {
        allocwarn("data buffer on client side");
        exit(-1);
    }

    while (1) {
        sent_bytes = send_msg(&conn, sock, buf, (const struct sockaddr *) server_addr, server_addr_size);
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
client_inet(const struct server_info *server_params) {
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

    server_addr = calloc(1, sizeof(struct sockaddr_in));
    if (server_addr == NULL) {
        allocwarn("server_addr address");
        exit(-1);
    }
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

    server_addr_size = malloc(sizeof(struct sockaddr_in));
    if (server_addr_size == NULL) {
        exit(-1);
    }
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

    buf = calloc(1, MAXDATABUFFLEN);
    if (buf == NULL) {
        allocwarn("data buffer on client side");
        exit(-1);
    }

    while (1) {
        sent_bytes = send_msg(&conn, sock, buf, (const struct sockaddr *) server_addr, server_addr_size);
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
send_msg(int *conn, int sock, char *buf, const struct sockaddr *server_addr, socklen_t *server_addr_size) {
    int len;
    int input_char;
    int sent_bytes;
    int need_send = 0;

    len = 0;
    putchar('>');
    while (!need_send) {
        input_char = getchar();

        switch (input_char) {
            case KEY_ESC:
                return -2;
            case KEY_ENTER:
                need_send = 1;
                break;
            default:
                buf[len++] = (char) input_char;
        }
    }

    sent_bytes = (int) sendto(
            sock,
            buf,
            len,
            MSG_NOSIGNAL,
            server_addr,
            *server_addr_size
    );
    if (DEBUG)
        printf(
                "[debug] %d bytes sent to '%s:%d'\n",
                sent_bytes,
                inet_ntoa(((struct sockaddr_in *) server_addr)->sin_addr),
                ntohs(((struct sockaddr_in *) server_addr)->sin_port)
        );

    if (sent_bytes < 0) {
        perror("sendto");
    }

    if (strcmp(buf, "exit") == 0) {
        printf("exiting client\n");
        return -2;
    }
    memset(buf, 0, len);
    return sent_bytes;
}

int
main(int argc, char **argv) {
    char c;
    struct server_info *server_params;
    server_params = malloc(server_info_size);
    if (server_params == NULL) {
        allocwarn("struct server_info");
        return -1;
    }

    if (argc == 1) {
        printf("usage: %s <unix|local> [descriptor path] or <inet> [host] [port]\n", argv[0]);
        return -1;
    }

    fill_server_info(server_params, argc, argv);

    switch (server_params->type) {
        case UNIX:
            printf("running unix client\n");
            client_unix(server_params);
            break;
        case INET:
            printf("running inet client\n");
            client_inet(server_params);
            break;
        case ERROR:
            printf("unknown server type\n");
            printf("usage: %s <unix|local> [descriptor path] or <inet> [host] [port]\n", argv[0]);
            break;
    }
    free(server_params);
    return 0;
}