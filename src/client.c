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
#include "network.h"

int
send_msg(int *conn, int sock, char *buf, const struct sockaddr *addr, ssize_t addr_size);

int
main() {
    int sock;
    int conn;
    int *DATABUFF;
    socklen_t addr_size;
    struct sockaddr_un *server;
    int sent_bytes = 0;

    sock = socket(SOCK_FAM, SOCK_TYPE, 0);
    server = calloc(1, sizeof(struct sockaddr_un));

    server->sun_family = SOCK_FAM;
    strncpy(server->sun_path, SOCK_PATH, sizeof(server->sun_path));

    if (sock == -1) {
        printf("error while creating socket\n");
        handle_socket_bind(sock);
        return -1;
    }

    addr_size = sizeof(server->sun_family) + strlen(server->sun_path) + 1;

    conn = connect(
            sock,
            (const struct sockaddr *) server,
            addr_size
    );

    if (conn == -1) {
        printf("\nerror while connecting to socket\n");
        handle_socket_bind(sock);
        return -1;
    }

    printf("connected to server\n");
    DATABUFF = calloc(1, MAXDATABUFFLEN);
    while (1) {
        sent_bytes = send_msg(&conn, sock, DATABUFF, (const struct sockaddr *) server, addr_size);
        if (sent_bytes == -2) {
            break;
        }
    }
    return 0;
}

int
send_msg(int *conn, int sock, char *buf, const struct sockaddr *addr, ssize_t addr_size) {
    int len;
    int input_char;
    ssize_t sent_bytes;

    len = 0;
    while ((input_char = getchar()) != KEY_ENTER) {
        if (input_char == KEY_ESC) {
            return -2;
        }
        buf[len++] = input_char;
    }
    len++;
    printf("sending data to server...\n");
    sent_bytes = sendto(sock, buf, len, MSG_NOSIGNAL, (const struct sockaddr *) addr, addr_size);
    printf("data sent to server, %zd bytes total with %d chars and data is: '%s'\n", sent_bytes, len, buf);
    memset(buf, 0, len);
    return sent_bytes;
}