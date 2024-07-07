//
// Created by Георгий Имешкенов on 11.12.2023.
//
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <printf.h>
#include <unistd.h>
#include <signal.h>

#include "unicode.h"

#include "network.h"
#include "packets.h"

#ifndef ARRAYLIST_H_INCLUDED

#endif


#define BACKLOG 16

void
serve_unix(const struct server_info_s *server_params) {
    int sock, sock_client;
    socklen_t *peer_addr_size, addr_size;
    struct sockaddr_un *server;
    struct sockaddr_un *peer_addr;
    char *in_buff;
    unsigned char *out_buff;

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

    *peer_addr_size = sizeof(struct sockaddr);
    sock_client = accept(sock, (struct sockaddr *) peer_addr, peer_addr_size);

    if (sock_client == -1) {
        printf("unable to accept connection: %s\n", server_params->socket_path);
        perror("accept");
        return;
    }
    printf("client '%s' connected\n", peer_addr->sun_path);

    in_buff = calloc(1, MAXNETWORKBUFFSIZE);
    out_buff = calloc(1, MAXNETWORKBUFFSIZE);
    if (in_buff == NULL || out_buff == NULL) {
        allocwarn("data buffer on server side");
        exit(-1);
    }

    puts("starting listening for incoming data");

    ArrayList_t *server_players = ArrayList(NULL);

    handle_client(
        server_players,
        sock_client,
        in_buff,
        out_buff,
        (struct sockaddr *) peer_addr,
        peer_addr_size
    );

    free(in_buff);
    free(out_buff);

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
    char *in_buff;
    unsigned char *out_buff;
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

    in_buff = calloc(1, MAXNETWORKBUFFSIZE);
    out_buff = calloc(1, MAXNETWORKBUFFSIZE);
    if (in_buff == NULL || out_buff == NULL) {
        allocwarn("data buffer on server side");
        exit(-1);
    }
    puts("starting listening for incoming data");

    ArrayList_t *server_players = ArrayList(NULL);

    if (IS_STREAM) {
        handle_client(server_players, peer_sock, in_buff, out_buff, (struct sockaddr *) peer_addr, peer_addr_size);
    } else {
        handle_client(server_players, host_sock, in_buff, out_buff, (struct sockaddr *) peer_addr, peer_addr_size);
    }

    free(in_buff);
    free(out_buff);
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

typedef struct server_player_network_s {
    socklen_t addr_len;
    struct sockaddr_in *address;
} server_player_network;

typedef struct server_player_obj {
    player_obj *player;
    server_player_network *network;
} server_player_obj;

void
construct_mp_player(server_player_obj **mp_player) {
    cmalloc_safe((*mp_player), sizeof(server_player_obj))
    cmalloc_safe((*mp_player)->player, sizeof(player_obj))
    cmalloc_safe((*mp_player)->player->pos, sizeof(player_pos))
    cmalloc_safe((*mp_player)->player->pos->x, sizeof(float))
    cmalloc_safe((*mp_player)->player->pos->y, sizeof(float))
    cmalloc_safe((*mp_player)->player->username, sizeof(UnicodeString))
    cmalloc_safe((*mp_player)->player->body, sizeof(SDL_FRect))
    cmalloc_safe((*mp_player)->network, sizeof(server_player_network))
    cmalloc_safe((*mp_player)->network->address, sizeof(struct sockaddr_in))
}

void
handle_client(
    ArrayList_t *server_players,
    int sock_client,
    char *in_buff,
    unsigned char *out_buff,
    struct sockaddr *client_addr,
    socklen_t *client_addr_size
) {
    unsigned char *result = NULL;
    char *prefix = NULL;
    server_player_obj *mp_player = NULL;
    size_t recv_bytes = 0;
    size_t sent_bytes = 0;

    while (1) {
        recv_bytes = recvfrom(
            sock_client,
            in_buff,
            MAXNETWORKBUFFSIZE,
            0,
            client_addr,
            client_addr_size
        );
        if (recv_bytes >= 2 * sizeof(int)) {
            if (recv_bytes < MAXNETWORKBUFFSIZE) {
                in_buff[++recv_bytes] = '\0';
            }

            cmalloc_safe(result, recv_bytes)
            memcpy(result, in_buff, recv_bytes);

            cmalloc_safe(prefix, NETWORK_BUFFER_OFFSET);

            in_addr_t *nclient_addr = &((struct sockaddr_in *) client_addr)->sin_addr.s_addr;
            printf(
                "[%s:%d] ",
                inet_ntop(AF_INET, (const void *) nclient_addr, prefix, *client_addr_size),
                ntohs(((struct sockaddr_in *) client_addr)->sin_port)
            );

            int packet_type = read_int_from_buff(&result);
            int player_id = read_int_from_buff(&result);

            unsigned char *packet_body = NULL;
            cmalloc_safe(packet_body, recv_bytes - 2 * sizeof(int));
            memcpy(packet_body, result, recv_bytes - 2 * sizeof(int));
            size_t packet_body_ptr = (size_t) packet_body;  // store packet body ptr to free it later and not care about offset calculation

            switch (packet_type) {
                case LOGIN:
                    construct_mp_player(&mp_player);

                    *mp_player->player->pos->x = read_float_from_buff(&packet_body);
                    *mp_player->player->pos->y = read_float_from_buff(&packet_body);
                    mp_player->player->username = read_into_unicode_string((char *) packet_body);
                    memcpy(mp_player->network->address, client_addr, sizeof(struct sockaddr_in));
                    memcpy(&mp_player->network->addr_len, client_addr_size, sizeof(size_t));

                    size_t last_player_index = server_players->size(server_players);
                    mp_player->player->id = (int) last_player_index;
                    server_players->push(server_players, mp_player);
                    printf(
                        "mp_player <id=%d, login='%s'> connected: \n", mp_player->player->id, packet_body
                    );
                    sent_bytes += write_int_to_buff(out_buff, sent_bytes, 0, mp_player->player->id);

                    sendto(
                        sock_client,
                        out_buff,
                        sent_bytes,
                        0,
                        (const struct sockaddr *) mp_player->network->address,
                        mp_player->network->addr_len
                    );
                    break;
                case MOVE:
                    mp_player = server_players->get(server_players, player_id);

                    if (mp_player == NULL) {
                        printf("unknown mp_player tried to access server: %d\n", player_id);
                        sendto(
                            sock_client,
                            out_buff,
                            sent_bytes,
                            0,
                            client_addr,
                            *client_addr_size
                        );
                        break;
                    }
                    *(mp_player->player->pos->x) = read_float_from_buff(&packet_body);
                    *(mp_player->player->pos->y) = read_float_from_buff(&packet_body);
                    printf("mp_player %s moved\n", compress_into_bytes_array(mp_player->player->username)->data);
                    sent_bytes += write_int_to_buff(out_buff, sent_bytes, 0, mp_player->player->id);
                    sent_bytes += write_float_to_buff(out_buff, sent_bytes, 0, *mp_player->player->pos->x);
                    sent_bytes += write_float_to_buff(out_buff, sent_bytes, 0, *mp_player->player->pos->y);
                    sendto(
                        sock_client,
                        out_buff,
                        sent_bytes,
                        0,
                        (const struct sockaddr *) mp_player->network->address,
                        mp_player->network->addr_len
                    );
                    break;
                case DISCONNECT:
                    mp_player = server_players->get(server_players, player_id);
                    if (mp_player == NULL) {
                        printf("unknown mp_player tried to access server: %d\n", player_id);
                        sendto(
                            sock_client,
                            out_buff,
                            sent_bytes,
                            0,
                            client_addr,
                            *client_addr_size
                        );
                        break;
                    }
                    sent_bytes = write_int_to_buff(out_buff, sent_bytes, 0, mp_player->player->id);
                    sendto(
                        sock_client,
                        out_buff,
                        sent_bytes,
                        0,
                        (const struct sockaddr *) mp_player->network->address,
                        mp_player->network->addr_len
                    );
                    printf("mp_player %s disconnected\n", packet_body);
                    break;
                default:
                    printf("unknown packet %d and payload: %s\n", (int) *result, result + 4);
                    break;
            }

            memset(in_buff, 0, recv_bytes);
            memset(out_buff, 0, sent_bytes);
            sent_bytes = 0;
            free((void *) packet_body_ptr);
            free(result - 2 * sizeof(int));
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
    malloc_safe(struct server_info_s *, server_params, server_info_size);

    if (argc == 1) {
        printf("usage: %s <unix|local> [descriptor path] or <inet> [host] [port]\n", argv[0]);
        return -1;
    }

    fill_server_info(server_params, argc, argv);

    switch (server_params->type) {
        case UNIX:
            printf("running unix server\n");
            serve_unix(server_params);
            break;
        case INET:
            printf("running inet server\n");
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
