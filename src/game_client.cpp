//
// Created by Георгий Имешкенов on 08.12.2023.
//

#include <iostream>
#include <random>

#define SDL_MAIN_HANDLED
#define LOCAL_ONLY

#include <cmath>
#include <SDL3/SDL.h>
#include <unistd.h>


extern "C" {
#include "client.c"
#include "packets.h"
}

using std::cerr, std::cin, std::cout;
using Clock = std::chrono::high_resolution_clock;
using TimeStamp = std::chrono::time_point<Clock>;

const unsigned int RED_MASK = 0xff000000;
const unsigned int GREEN_MASK = 0x00ff0000;
const unsigned int BLUE_MASK = 0x0000ff00;
const unsigned int ALPHA_MASK = 0x000000ff;

bool running = true;
struct client_s *network_client;

typedef void (*quit_handler_t)();

typedef void (*keydown_handler_t)(SDL_Scancode, volatile client_game_state *);

typedef void (*keyup_handler_t)(SDL_Scancode, volatile client_game_state *);

struct {
    quit_handler_t quit;
    keydown_handler_t keydown;
    keyup_handler_t keyup;
} event_handlers;

//const unsigned int BACKGROUND = 0xffffffff;
const unsigned int BACKGROUND = 0x00000000;

//player_obj *player;
SDL_Surface *numbers_atlas;

//struct window_dims_s *window_dims;

float PLAYER_VELOCITY = 300.0f;
float PLAYER_TARGET_VELOCITY = 300.0f;

typedef std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::duration<long long, std::nano >> time_point;

size_t
write_string_to_network_buff(const char *data) {
    size_t data_len = strlen(data);
    size_t rem_len = data_len;

    if (network_client->buffer->fill == MAXNETWORKBUFFSIZE) return data_len;

    for (size_t i = network_client->buffer->fill; i < MAXNETWORKBUFFSIZE && rem_len > 0; i++, rem_len--) {
        *(network_client->buffer->data + i) = *(data + i - NETWORK_BUFFER_OFFSET);
    }

    network_client->buffer->fill = (rem_len > 0) ? MAXNETWORKBUFFSIZE : network_client->buffer->fill + data_len;

    return rem_len;
}

size_t
write_player_to_network_buff(const player_obj *player_obj) {
    if (
        network_client->buffer->fill == MAXNETWORKBUFFSIZE ||
        (network_client->buffer->fill - 4 * int_size + 1) < 0
        ) {
        return 1;
    }

    write_int_to_network_client(network_client, player_obj->id);        // 4
    write_float_to_network_client(network_client, *player_obj->pos->x); // 4
    write_float_to_network_client(network_client, *player_obj->pos->y); // 4
    return 0;
}

void SDL_SetRenderDrawColor(SDL_Renderer *, unsigned int);

void connect_to_server(volatile client_game_state *game_state, player_obj *);
//12900112
//719700991

//7724799
//2942756607

int last_entity_id = 0;

bool keyread_locked = false;

void
handle_keydown(SDL_Scancode keycode, volatile client_game_state *game_state) {
    player_obj *player = game_state->player;
    window_dims_s *window_dims = game_state->window_dims;
    int player_changed = 1;

    switch (keycode) {
        case SDL_SCANCODE_ESCAPE: {
            running = false;
            break;
        }
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_W:
//        case 1094:
            *player->pos->y = std::clamp(
                (*player->pos->y - 1 * PLAYER_TARGET_VELOCITY),
                0.0f,
                (static_cast<float> (window_dims->height) - player->body->h)
            );
            break;
        case SDL_SCANCODE_DOWN:
        case SDL_SCANCODE_S:
//        case 1099:
            *player->pos->y = std::clamp(
                (*player->pos->y + 1 * PLAYER_TARGET_VELOCITY),
                0.0f,
                (static_cast<float> (window_dims->height) - player->body->h)
            );
            break;
        case SDL_SCANCODE_LEFT:
        case SDL_SCANCODE_A:
//        case 1092:
            *player->pos->x = std::clamp(
                (*player->pos->x - 1 * PLAYER_TARGET_VELOCITY),
                0.0f,
                (static_cast<float> (window_dims->width) - player->body->w)
            );
            break;
        case SDL_SCANCODE_RIGHT:
        case SDL_SCANCODE_D:
//        case 1074:
            *player->pos->x = std::clamp(
                (*player->pos->x + 1 * PLAYER_TARGET_VELOCITY),
                0.0f,
                (static_cast<float> (window_dims->width) - player->body->w)
            );
            break;
        case SDL_SCANCODE_LSHIFT:
            PLAYER_TARGET_VELOCITY = PLAYER_VELOCITY * 16.0f;
            break;
        case SDL_SCANCODE_LCTRL:
            PLAYER_TARGET_VELOCITY = PLAYER_VELOCITY / 4.0f;
            break;
        case SDL_SCANCODE_SPACE:
            break;
        default:
//            printf("%d\n", keycode);
            player_changed = 0;
            break;
    }
    if (player_changed) {
        write_int_to_network_client(network_client, MOVE);
        write_player_to_network_buff(player);
        game_state->network_used = 1;
    }
}

void
handle_keyup(SDL_Scancode keycode, volatile client_game_state *game_state) {
    switch (keycode) {
        case SDL_SCANCODE_LSHIFT:
        case SDL_SCANCODE_LCTRL:
            PLAYER_TARGET_VELOCITY = PLAYER_VELOCITY;
            break;
        case SDL_SCANCODE_R:
            game_state->network_used = 1;
            connect_to_server(game_state, game_state->player);
            break;
        default:
            break;
    }
}

void
purge_network_buff() {
    memset(network_client->buffer->data + NETWORK_BUFFER_OFFSET, '\0', MAXNETWORKBUFFSIZE - NETWORK_BUFFER_OFFSET);
    network_client->buffer->fill = NETWORK_BUFFER_OFFSET;
}

void
connect_to_server(volatile client_game_state *game_state, player_obj *player) {
    CompressedUnicodeString *compressed_username = compress_into_bytes_array(player->username);
    write_int_to_network_client(network_client, LOGIN); // 4
    write_player_to_network_buff(player);  // 16
    memmove(
        network_client->buffer->data + network_client->buffer->fill,
        compressed_username->data,
        compressed_username->len
    );
    network_client->buffer->fill += compressed_username->len;
    game_state->network_queue->push_back(network_client);
//    send_message_with_client_only(network_client);

    purge_network_buff();
    free(compressed_username);
    keyread_locked = false;
}

void
disconnect_from_server(player_obj *player) {
    CompressedUnicodeString *compressed_username = compress_into_bytes_array(player->username);
    write_int_to_network_client(network_client, DISCONNECT);
    write_int_to_network_client(network_client, player->id);
    memmove(
        network_client->buffer->data + network_client->buffer->fill,
        compressed_username->data,
        compressed_username->len
    );
    network_client->buffer->fill += compressed_username->len;
    send_message_with_client_only(network_client);
    purge_network_buff();
    free(compressed_username);
}

void
main_loop(
    SDL_Renderer *renderer,
    volatile client_game_state *game_state
) {
//    player_obj *player = game_state->player;
    event_handlers.keydown = handle_keydown;
    event_handlers.keyup = handle_keyup;

    time_point now = std::chrono::high_resolution_clock::now();
    time_point last_update = std::chrono::high_resolution_clock::now();
    std::chrono::duration<long long, std::nano> diff{};

    long long time_counter = 0;
    long long time_counter_threshold = 1e9;
    long long updates = 0;
    SDL_Event event;

    const Uint8 *keyboard_state;
    double target_ups = 600.0;
    auto target_update_ns = static_cast<long long>(1e9 / target_ups);
    double world_timer = 0.0;

    PLAYER_VELOCITY = (float) (PLAYER_VELOCITY * 60 * 1e2 / (double) target_update_ns);
    PLAYER_TARGET_VELOCITY = PLAYER_VELOCITY;
    connect_to_server(game_state, game_state->player);
    last_update = std::chrono::high_resolution_clock::now();

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    long long last_local_tick_counter = 0;

    while (running) {
        while (SDL_PollEvent(&event) != 0) {
            switch (event.type) {
                case SDL_EVENT_KEY_UP:
                    keyread_locked = true;
                    event_handlers.keyup(event.key.keysym.scancode, game_state);
                    break;
                case SDL_EVENT_KEY_DOWN:
                    keyread_locked = true;
                    break;
            }
        }

        keyboard_state = SDL_GetKeyboardState(nullptr);
        if (keyboard_state[SDL_SCANCODE_ESCAPE]) {
            running = false;
            disconnect_from_server(game_state->player);
        }

        if (keyboard_state[SDL_SCANCODE_DOWN] || keyboard_state[SDL_SCANCODE_S]) {
            event_handlers.keydown(SDL_SCANCODE_DOWN, game_state);
        }

        if (keyboard_state[SDL_SCANCODE_LEFT] || keyboard_state[SDL_SCANCODE_A]) {
            event_handlers.keydown(SDL_SCANCODE_LEFT, game_state);
        }

        if (keyboard_state[SDL_SCANCODE_RIGHT] || keyboard_state[SDL_SCANCODE_D]) {
            event_handlers.keydown(SDL_SCANCODE_RIGHT, game_state);
        }

        if (keyboard_state[SDL_SCANCODE_UP] || keyboard_state[SDL_SCANCODE_W]) {
            event_handlers.keydown(SDL_SCANCODE_UP, game_state);
        }

        if (keyboard_state[SDL_SCANCODE_R]) {
            event_handlers.keydown(SDL_SCANCODE_R, game_state);
        }

        if (keyboard_state[SDL_SCANCODE_LSHIFT]) {
            event_handlers.keydown(SDL_SCANCODE_LSHIFT, game_state);
        } else {
            event_handlers.keyup(SDL_SCANCODE_LSHIFT, game_state);
        }

        if (keyboard_state[SDL_SCANCODE_LCTRL]) {
            event_handlers.keydown(SDL_SCANCODE_LCTRL, game_state);
        } else {
            event_handlers.keyup(SDL_SCANCODE_LCTRL, game_state);
        }

        if (game_state->network_used && keyread_locked) {
            send_message_with_client_only(network_client);
            purge_network_buff();
            game_state->network_used = 0;
            keyread_locked = false;
        }

        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, game_state->player->color);
        SDL_RenderFillRect(renderer, game_state->player->body);

        SDL_SetRenderDrawColor(renderer, BACKGROUND);
        SDL_RenderPresent(renderer);

        now = std::chrono::high_resolution_clock::now();

        diff = now - last_update;
        time_counter += diff.count();

        if (time_counter >= time_counter_threshold - target_update_ns) {
            time_counter = 0.0;
        }

        usleep(
            (useconds_t) std::max(
                std::max(
                    (long long) ((double) (target_update_ns - diff.count()) / 1e3),
                    0ll
                ),
                (long long) ((double) target_update_ns / 1e3)
            )
        );
        world_timer += (double) target_update_ns / 1e9;
        last_update = now;
    }
}

void
SDL_SetRenderDrawColor(SDL_Renderer *renderer, unsigned int color) {
    SDL_SetRenderDrawColor(
        renderer,
        (color & RED_MASK) >> 24,
        (color & GREEN_MASK) >> 16,
        (color & BLUE_MASK) >> 8,
        color & ALPHA_MASK
    );
}

struct window_dims_s *
prepare_display() {
    puts("preparing window...");
    SDL_DisplayID display_id = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode *displayMode = SDL_GetDesktopDisplayMode(display_id);
    int width = displayMode->w / 2;
    int height = displayMode->h / 2;
    puts("allocating dims...");
    auto *window_dims = reinterpret_cast<window_dims_s *>(malloc(sizeof(struct window_dims_s)));
    window_dims->width = width;
    window_dims->height = height;
    puts("window prepared");
    return window_dims;
}

void
run_game_client(int argc, char **argv) {
    struct server_info_s *server_params;
    pthread_t *network_thread = malloc_safe(pthread_t *, network_thread, sizeof(pthread_t));
    pthread_attr_t *attr = nullptr;
//    char const *argv[] = {nullptr, "inet", "127.0.0.1", "10312"};
//    char const *argv[] = {nullptr, "inet", "161.35.71.57", "10312"};
    puts("server connection info:");
    for (int i = 0; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    putchar('\n');
    server_params = reinterpret_cast<server_info_s *>(malloc(server_info_size));
    if (server_params == nullptr) {
        allocwarn("struct server_info_s");
        exit(-1);
    }

    if (argc == 1) {
        printf("usage: %s <unix|local> [descriptor path] or <inet> [host] [port] <username>\n", argv[0]);
        exit(-1);
    }

    fill_server_info(server_params, argc, const_cast<char **>(argv));
    printf("server to connect to: %s:%d\n", server_params->address, server_params->port);

    puts("initing SDL..");
    if (SDL_Init(SDL_INIT_VIDEO)) {
        cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return;
    }
    puts("done");
    volatile auto *game_state = static_cast<client_game_state *>(malloc(sizeof(client_game_state)));
    struct window_dims_s *window_dims = prepare_display();
    game_state->window_dims = window_dims;
    puts("constructing player...");
    game_state->player = reinterpret_cast<player_obj *>(malloc(sizeof(player_obj_s)));
    player_obj *player = game_state->player;
    if (player == nullptr) {
        printf("unable to init player");
        exit(-1);
    }
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution color(0, 0xffffff);
    std::uniform_int_distribution initX(0.0f, static_cast<float>(window_dims->width));
    std::uniform_int_distribution initY(0.0f, static_cast<float>(window_dims->height));

    player->color = 12900112 << 8 | 0xff;
    player->body = new SDL_FRect{
        static_cast<float>(window_dims->width / 2.0),
        static_cast<float>(window_dims->height / 2.0),
        8.0,
        8.0
    };
    player->pos = new player_pos{
        &player->body->x,
        &player->body->y
    };
    player->id = last_entity_id++;
    calloc_safe(UnicodeString *, player->username, 1, sizeof(UnicodeString));
    char const *username_raw = argv[argc - 1];
    player->username = read_into_unicode_string(username_raw);

    puts("player constructed");
    printf("Created player: %s\n", compress_into_bytes_array(player->username)->data);
    printf("player color: %u\n", player->color);

    printf("loading textures...");
    numbers_atlas = SDL_LoadBMP(RESOURCES_DIR"numbers.bmp");
    puts("done");

    puts("setting up window and renderer...");
    SDL_Window *window = SDL_CreateWindow(
        "game client",
        static_cast<int>(window_dims->width),
        static_cast<int>(window_dims->height),
        SDL_EVENT_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS
    );
    SDL_Renderer *renderer = SDL_CreateRenderer(
        window,
        nullptr,
        SDL_RENDERER_PRESENTVSYNC
    );
    SDL_SetRenderDrawColor(renderer, 0xffffffff);
    puts("window set up");

    puts("constructing network client...");
    network_client = create_client(server_params, MAXNETWORKBUFFSIZE, nullptr);
    if (network_client == nullptr) {
        std::cout << "unable to init network client, exiting" << std::endl;
        exit(-1);
    }
    game_state->network_client = network_client;
    game_state->network_queue = Deque(nullptr);
    pthread_create(
        network_thread,
        attr,
        reinterpret_cast<void *(*)(void *)>(start_network_thread),
        (void *) game_state
    );
    puts("network thread started");
    puts("network client constructed");

    main_loop(renderer, game_state);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    free(network_client);
    SDL_Quit();
}

int
main([[maybe_unused]] int argc, char **argv) {
    run_game_client(argc, argv);
}
