//
// Created by Георгий Имешкенов on 11.12.2023.
//
#include <stdio.h>
#include <stdlib.h>

#define DEBUG 1
#define SOCK_PATH "/tmp/gengine.sock"
#define INET_ADDR "127.0.0.1"
#define INET_PORT 10312
#define SOCK_FAM AF_INET
#define SOCK_TYPE SOCK_STREAM
#define MAXDATABUFFLEN 1024
#define KEY_ENTER 10
#define KEY_ESC 27
#define KEY_SPACE 32
#define allocwarn(target) printf("unable to alloc mem for '"#target"'\n")

#define IS_STREAM (server_params->sock_type == SOCK_STREAM)
#define BIND(SOCK, ADDR) bind(SOCK, (struct sockaddr *) ADDR, sizeof(struct sockaddr_in))

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define REUSEADDR SO_REUSEPORT
#else
#define REUSEADDR SO_REUSEADDR
#endif

enum ServerType {
    ERROR = -1,
    UNIX = 0,
    LOCAL = 0,
    INET = 2,
};

struct server_info {
    enum ServerType type;
    char socket_path[32];           // nevertheless default value is "/tmp/gengine.sock" we allow 32-byte string (including null-terminator)
    char address[INET6_ADDRSTRLEN]; // we want to have enough size for both IPv4 and IPv6 addresses
    short port;
    int sock_fam;
    int sock_type;
};

const size_t server_info_size = sizeof(struct server_info);

void
fill_server_info(struct server_info *server_info, int argc, char **argv) {
    char *type = argv[1];

//    initial fill with default params
    strncpy(server_info->socket_path, SOCK_PATH, strlen(SOCK_PATH));
    strncpy(server_info->address, INET_ADDR, strlen(INET_ADDR));
    server_info->port = INET_PORT;

    switch (argc) {
        case 2: {
            if (strcmp(type, "unix") == 0 || strcmp(type, "local") == 0) {
                server_info->type = UNIX | LOCAL;
                server_info->sock_fam = AF_UNIX;
                server_info->sock_type = SOCK_STREAM;
            } else if (strcmp(type, "inet") == 0) {
                server_info->type = INET;
                server_info->sock_fam = AF_INET;
                server_info->sock_type = SOCK_DGRAM;
            } else {
                server_info->type = ERROR;
            }
            break;
        }
        case 3: {
            if (strcmp(type, "unix") == 0 || strcmp(type, "local") == 0) {
                server_info->type = UNIX | LOCAL;
                server_info->sock_fam = AF_UNIX;
                server_info->sock_type = SOCK_STREAM;
                strncpy(server_info->socket_path, argv[2], strlen(argv[2]));
            } else if (strcmp(type, "inet") == 0) {
                server_info->type = INET;
                server_info->sock_fam = AF_INET;
                server_info->sock_type = SOCK_DGRAM;
                strncpy(server_info->address, argv[2], strlen(argv[2]));
            } else {
                server_info->type = ERROR;
            }
            break;
        }
        default:
            if (strcmp(type, "unix") == 0 || strcmp(type, "local") == 0) {
                server_info->type = UNIX | LOCAL;
                server_info->sock_fam = AF_UNIX;
                server_info->sock_type = SOCK_STREAM;
                strncpy(server_info->socket_path, argv[2], strlen(argv[2]));
            } else if (strcmp(type, "inet") == 0) {
                server_info->type = INET;
                server_info->sock_fam = AF_INET;
                server_info->sock_type = SOCK_DGRAM;
                strncpy(server_info->address, argv[2], strlen(argv[2]));
                server_info->port = (short) strtol(argv[3], NULL, 10);
            } else {
                server_info->type = ERROR;
            }
            break;
    }
}

int
send_msg(int *conn, int sock, char *buf, const struct sockaddr *server_addr, socklen_t *server_addr_size);

void
handle_client(int sock_client, char *buff, struct sockaddr *client_addr, socklen_t *client_addr_size);