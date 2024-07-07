//
// Created by Георгий Имешкенов on 11.12.2023.
//
#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>

#include "array_list.h"
#include "unicode.h"

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

struct buffer_s {
    unsigned char *data;
    size_t fill;
    size_t max_len;
};

struct client_s {
    int *socket;
    struct sockaddr_in *host_addr;
    socklen_t *host_addr_len;
    struct buffer_s *buffer;
};

const size_t client_size = sizeof(struct client_s);

//const size_t NETWORK_BUFFER_OFFSET = sizeof(in_addr_t) + sizeof(in_port_t);
const size_t NETWORK_BUFFER_OFFSET = 0;
const size_t byte_len = 8;
const size_t int_size = sizeof(int);
const size_t float_size = sizeof(float);

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
send_message(const struct client_s *client, unsigned char *buf);

int
send_message_and_flush(const struct client_s *client, unsigned char *buf);

int
send_msg_internal(int *conn, int sock, char *buf, const struct sockaddr *server_addr, socklen_t *server_addr_size);

size_t
write_int_to_buff(unsigned char *buff, size_t offset, size_t limit, int num);

size_t
write_float_to_buff(unsigned char *buff, size_t offset, size_t limit, float num);


size_t
write_int_to_network_client(struct client_s *network_client, int num) {
    if (network_client->buffer->fill - int_size < 0) return 1;

    network_client->buffer->fill = write_int_to_buff(
            network_client->buffer->data,
            network_client->buffer->fill,
            0,
            num
    );
    return network_client->buffer->fill;
}

size_t
write_int_to_buff(unsigned char *buff, size_t offset, size_t limit, int num) {
    if (limit > 0 && limit - int_size < 0) return 1;
    *(buff + offset) = num;
    return offset + int_size;
}

size_t
write_float_to_network_client(struct client_s *network_client, float num) {
    if (network_client->buffer->fill - int_size < 0) return 1;

    network_client->buffer->fill = write_float_to_buff(
        network_client->buffer->data,
        network_client->buffer->fill,
        0,
        num
    );
    return network_client->buffer->fill;
}

size_t
write_float_to_buff(unsigned char *buff, size_t offset, size_t limit, float num) {
    if (limit > 0 && limit - float_size < 0) return 1;
    *(buff + offset) = num;
    return offset + float_size;
}

int
read_int_from_network_client(struct client_s *network_client) {
    int res = 0;
    for (int i = 0; i < int_size; network_client->buffer->fill++) {
        *(&res + i++) = *(network_client->buffer->data++);
    }
    return res;
}

int
read_int_from_buff(unsigned char **buff) {
    int res = (int) **buff;
    *buff += int_size;
    return res;
}

float
read_float_from_network_client(struct client_s *network_client) {
    float res = 0.0f;
    for (int i = 0; i < int_size; network_client->buffer->fill++) {
        *(&res + i++) = *(network_client->buffer->data++);
    }
    return res;
}

float
read_float_from_buff(unsigned char **buff) {
    float res = (float) **buff;
    *buff += float_size;
    return res;
}

void
handle_client(
    ArrayList_t *server_players,
    int sock_client,
    char *in_buff,
    unsigned char *out_buff,
    struct sockaddr *client_addr,
    socklen_t *client_addr_size
);

typedef struct player_pos_s {
    float *x;
    float *y;
} player_pos;

typedef struct player_obj_s {
    int id;
    unsigned int color;
    player_pos *pos;
    UnicodeString *username;
    SDL_FRect *body;
} player_obj;