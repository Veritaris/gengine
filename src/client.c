//
// Created by Георгий Имешкенов on 11.12.2023.
//
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <printf.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "network.h"

int
send_msg(int *conn, int sock, char *buf, const struct sockaddr *server_addr, socklen_t *server_addr_size);

int
client_unix(const char *socket_path) {
    int sock;
    int conn;
    char *buf;
    socklen_t *server_addr_size;
    struct sockaddr_un *server_addr;
    int sent_bytes;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);

    server_addr = malloc(sizeof(struct sockaddr_un));
    if (server_addr == NULL) {
        allocwarn("server_addr address");
        exit(-1);
    }

    server_addr->sun_family = AF_UNIX;
    strncpy(server_addr->sun_path, socket_path, sizeof(server_addr->sun_path));

    if (sock == -1) {
        printf("error while creating socket\n");
        handle_socket_bind(sock);
        return -1;
    }

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
        handle_socket_connect(sock);
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
client_inet(char *host, int port) {
    int sock;
    int conn;
    char *buf;
    socklen_t *server_addr_size;
    struct sockaddr_in *server_addr;
    int sent_bytes;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("error while creating socket\n");
        handle_socket_bind(sock);
        return -1;
    }

    server_addr = calloc(1, sizeof(struct sockaddr_in));
    if (server_addr == NULL) {
        allocwarn("server_addr address");
        exit(-1);
    }
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);
    inet_aton(host, &(server_addr->sin_addr));

    server_addr_size = malloc(sizeof(struct sockaddr_in));
    if (server_addr_size == NULL) {
        exit(-1);
    }
    *server_addr_size = sizeof(struct sockaddr_in);

    printf("connecting to server '%s:%d'...\n", inet_ntoa(server_addr->sin_addr), ntohs(server_addr->sin_port));
    conn = connect(
            sock,
            (const struct sockaddr *) server_addr,
            sizeof(struct sockaddr_in)
    );

    if (conn == -1) {
        printf("error while connecting to server\n");
        handle_socket_connect(sock);
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
send_msg(int *conn, int sock, char *buf, const struct sockaddr *server_addr, socklen_t *server_addr_size) {
    int len;
    int input_char;
    int sent_bytes;

    len = 0;
    putchar('>');
    while ((input_char = getchar()) != KEY_ENTER) {
        if (input_char == KEY_ESC) {
            return -2;
        }
        buf[len++] = (char) input_char;
    }

    sent_bytes = (int) sendto(
            sock,
            buf,
            len,
            MSG_NOSIGNAL,
            server_addr,
            *server_addr_size
    );
    if (DEBUG) printf("[debug] %d bytes sent\n", sent_bytes);

    if (sent_bytes < 0) {
        handle_send_error();
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
    enum ServerType connect_type = ERROR;
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
                connect_type = UNIX | LOCAL;
            } else if (strcmp(type, "inet") == 0) {
                connect_type = INET;
            } else {
                connect_type = ERROR;
            }
            break;
        }
        case 3: {
            type = argv[1];
            if (strcmp(type, "unix") == 0 || strcmp(type, "local") == 0) {
                connect_type = UNIX | LOCAL;
                socket_path = argv[2];
            } else if (strcmp(type, "inet") == 0) {
                connect_type = INET;
                host = argv[2];
            } else {
                connect_type = ERROR;
            }
            break;
        }
        case 4: {
            type = argv[1];
            if (strcmp(type, "unix") == 0 || strcmp(type, "local") == 0) {
                connect_type = UNIX | LOCAL;
                socket_path = argv[2];
            } else if (strcmp(type, "inet") == 0) {
                connect_type = INET;
                host = argv[2];
                port = (int) strtol(argv[3], NULL, 10);
            } else {
                connect_type = ERROR;
            }
            break;
        }
        default:
            type = argv[1];
            if (strcmp(type, "unix") == 0 || strcmp(type, "local") == 0) {
                connect_type = UNIX | LOCAL;
                socket_path = argv[2];
            } else if (strcmp(type, "inet") == 0) {
                connect_type = INET;
                host = argv[2];
                port = (int) strtol(argv[3], NULL, 10);
            } else {
                connect_type = ERROR;
            }
            break;
    }

    switch (connect_type) {
        case ERROR:
            printf("unknown server type\n");
            printf("usage: %s <unix|local> [descriptor path] or <inet> [host] [port]\n", argv[0]);
            break;
        case UNIX:
            client_unix(socket_path);
            break;
        case INET:
            client_inet(host, port);
            break;
    }
    return 0;
}