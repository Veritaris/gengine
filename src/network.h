//
// Created by Георгий Имешкенов on 11.12.2023.
//
#include <stdio.h>
#include <stdlib.h>

#define DEBUG 1
#define SOCK_PATH "/tmp/gengine.sock\0"
#define INET_ADDR "127.0.0.1\0"
#define INET_PORT 10312
#define SOCK_FAM AF_INET
#define SOCK_TYPE SOCK_STREAM
#define MAXNETWORKBUFFSIZE 65536
#define KEY_ENTER 10
#define KEY_ESC 27
#define KEY_SPACE 32
#define allocwarn(target) printf("unable to alloc mem for '"#target"'\n")

#define malloc_save(cast, var, var_size) \
    var = (cast) malloc(var_size); \
    if (var == NULL) {                                     \
        printf("failed to malloc for '"#var"' in '%s' at '%d'\n", __FILE__, __LINE__-2);                                        \
        exit(-1);\
    }
#define cmalloc_save(var, var_size) \
    var = malloc(var_size); \
    if (var == NULL) {                                     \
        printf("failed to malloc for '"#var"' in '%s' at '%d'\n", __FILE__, __LINE__-2);                                        \
        exit(-1);\
    }
#define calloc_save(cast, var, amount, var_size) \
    var = (cast) calloc(amount, var_size); \
    if (var == NULL) {                                     \
        printf("failed to calloc for '"#var"' in '%s' at '%d'\n", __FILE__, __LINE__-2);                                        \
        exit(-1);\
    }
#define ccalloc_save(var, amount, var_size) \
    var = calloc(amount, var_size); \
    if (var == NULL) {                                     \
        printf("failed to calloc for '"#var"' in '%s' at '%d'\n", __FILE__, __LINE__-2);                                        \
        exit(-1);\
    }
#define IS_STREAM (server_params->sock_type == SOCK_STREAM)
#define BIND(SOCK, ADDR) bind(SOCK, (struct sockaddr *) ADDR, sizeof(struct sockaddr_in))

enum ServerType {
    ERROR = -1,
    UNIX = 0,
    INET = 2,
};

struct server_info_s {
    enum ServerType type;
    char socket_path[32];           // nevertheless default value is "/tmp/gengine.sock" we allow 32-byte string (including null-terminator)
    char address[INET6_ADDRSTRLEN]; // we want to have enough size for both IPv4 and IPv6 addresses
    short port;
    int sock_fam;
    int sock_type;
};

const size_t server_info_size = sizeof(struct server_info_s);

struct client_s {
    int *socket;
    struct sockaddr_in *host_addr;
    socklen_t *host_addr_len;
};

const size_t client_size = sizeof(struct client_s);

//const size_t NETWORK_BUFFER_OFFSET = sizeof(in_addr_t) + sizeof(in_port_t);
const size_t NETWORK_BUFFER_OFFSET = 0;

void
fill_server_info(struct server_info_s *server_info, int argc, char **argv) {
    char *type = argv[1];

//    initial fill with default params
    strncpy(server_info->socket_path, SOCK_PATH, strlen(SOCK_PATH) + 1);
    strncpy(server_info->address, INET_ADDR, strlen(INET_ADDR) + 1);
    server_info->port = INET_PORT;

    switch (argc) {
        case 2: {
            if (strcmp(type, "unix") == 0 || strcmp(type, "local") == 0) {
                server_info->type = (enum ServerType) UNIX;
                server_info->sock_fam = AF_UNIX;
                server_info->sock_type = SOCK_STREAM;
            } else if (strcmp(type, "inet") == 0) {
                server_info->type = (enum ServerType) INET;
                server_info->sock_fam = AF_INET;
                server_info->sock_type = SOCK_DGRAM;
            } else {
                server_info->type = (enum ServerType) ERROR;
            }
            break;
        }
        case 3: {
            if (strcmp(type, "unix") == 0 || strcmp(type, "local") == 0) {
                server_info->type = (enum ServerType) UNIX;
                server_info->sock_fam = AF_UNIX;
                server_info->sock_type = SOCK_STREAM;
                strncpy(server_info->socket_path, argv[2], strlen(argv[2]) + 1);
            } else if (strcmp(type, "inet") == 0) {
                server_info->type = (enum ServerType) INET;
                server_info->sock_fam = AF_INET;
                server_info->sock_type = SOCK_DGRAM;
                strncpy(server_info->address, argv[2], strlen(argv[2]) + 1);
            } else {
                server_info->type = (enum ServerType) ERROR;
            }
            break;
        }
        default:
            if (strcmp(type, "unix") == 0 || strcmp(type, "local") == 0) {
                server_info->type = (enum ServerType) UNIX;
                server_info->sock_fam = AF_UNIX;
                server_info->sock_type = SOCK_STREAM;
                strncpy(server_info->socket_path, argv[2], strlen(argv[2]) + 1);
            } else if (strcmp(type, "inet") == 0) {
                server_info->type = (enum ServerType) INET;
                server_info->sock_fam = AF_INET;
                server_info->sock_type = SOCK_DGRAM;
                strncpy(server_info->address, argv[2], strlen(argv[2]) + 1);
                server_info->port = (short) strtol(argv[3], NULL, 10);
            } else {
                server_info->type = (enum ServerType) ERROR;
            }
            break;
    }
}

int
send_message(const struct client_s *client, char *buf);

int
send_message_and_flush(const struct client_s *client, char *buf);

int
send_msg_internal(int *conn, int sock, char *buf, const struct sockaddr *server_addr, socklen_t *server_addr_size);

void
handle_client(int sock_client, char *buff, struct sockaddr *client_addr, socklen_t *client_addr_size);