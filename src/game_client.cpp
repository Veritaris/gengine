//
// Created by Георгий Имешкенов on 08.12.2023.
//

#include <iostream>
#include <random>

#define SDL_MAIN_HANDLED
#define LOCAL_ONLY

#include <cmath>
#include <SDL3/SDL.h>
#include <libc.h>


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
struct window_dims_s {
    unsigned int width;
    unsigned int height;
};
typedef struct client_game_state {
    window_dims_s *window_dims;
    player_obj *player;
} client_game_state;

typedef void (*quit_handler_t)();

typedef void (*keydown_handler_t)(SDL_Keycode, client_game_state *);

typedef void (*keyup_handler_t)(SDL_Keycode, client_game_state *);

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
//    network_client->buffer->fill += int_size + 2 * float_size;
    return 0;
}

void SDL_SetRenderDrawColor(SDL_Renderer *, unsigned int);

void connect_to_server(player_obj *);
//12900112
//719700991

//7724799
//2942756607

int last_entity_id = 0;

void
handle_keydown(SDL_Keycode keycode, client_game_state *game_state) {
    player_obj *player = game_state->player;
    window_dims_s *window_dims = game_state->window_dims;
    int player_changed = 1;

    switch (keycode) {
        case SDLK_ESCAPE: {
            running = false;
            break;
        }
        case SDLK_UP:
        case SDLK_w:
        case 1094:
            *player->pos->y = std::clamp(
                    (*player->pos->y - 1 * PLAYER_TARGET_VELOCITY),
                    0.0f,
                    (static_cast<float> (window_dims->height) - player->body->h)
            );
            break;
        case SDLK_DOWN:
        case SDLK_s:
        case 1099:
            *player->pos->y = std::clamp(
                    (*player->pos->y + 1 * PLAYER_TARGET_VELOCITY),
                    0.0f,
                    (static_cast<float> (window_dims->height) - player->body->h)
            );
            break;
        case SDLK_LEFT:
        case SDLK_a:
        case 1092:
            *player->pos->x = std::clamp(
                    (*player->pos->x - 1 * PLAYER_TARGET_VELOCITY),
                    0.0f,
                    (static_cast<float> (window_dims->width) - player->body->w)
            );
            break;
        case SDLK_RIGHT:
        case SDLK_d:
        case 1074:
            *player->pos->x = std::clamp(
                    (*player->pos->x + 1 * PLAYER_TARGET_VELOCITY),
                    0.0f,
                    (static_cast<float> (window_dims->width) - player->body->w)
            );
            break;
        case SDLK_r:
        case SDL_SCANCODE_R:
            player_changed = 0;
            connect_to_server(player);
            break;
        case SDLK_LSHIFT:
            PLAYER_TARGET_VELOCITY = PLAYER_VELOCITY * 4.0f;
            break;
        case SDLK_LCTRL:
            PLAYER_TARGET_VELOCITY = PLAYER_VELOCITY / 4.0f;
            break;
        case SDLK_SPACE:
            break;
        default:
            printf("%d\n", keycode);
            player_changed = 0;
            break;
    }
    if (player_changed) {
        write_int_to_network_client(network_client, MOVE);
        write_player_to_network_buff(player);
    }
}

void
handle_keyup(SDL_Keycode keycode, client_game_state *game_state) {
    switch (keycode) {
        case SDLK_LSHIFT:
        case SDLK_LCTRL:
            PLAYER_TARGET_VELOCITY = PLAYER_VELOCITY;
            break;
        default:
            printf("%d\n", keycode);
            break;
    }
}

void
purge_network_buff() {
    memset(network_client->buffer->data + NETWORK_BUFFER_OFFSET, '\0', MAXNETWORKBUFFSIZE - NETWORK_BUFFER_OFFSET);
    network_client->buffer->fill = NETWORK_BUFFER_OFFSET;
}

void
connect_to_server(player_obj *player) {
    CompressedUnicodeString *compressed_username = compress_into_bytes_array(player->username);
    write_int_to_network_client(network_client, LOGIN); // 4
    write_player_to_network_buff(player);  // 16
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
        client_game_state *game_state
) {
    player_obj *player = game_state->player;
    int network_used = 0;
    event_handlers.keydown = handle_keydown;
    event_handlers.keyup = handle_keyup;

    time_point now = std::chrono::high_resolution_clock::now();
    time_point last_update = std::chrono::high_resolution_clock::now();
    std::chrono::duration<long long, std::nano> diff{};

    long long time_counter = 0;
    long long time_counter_threshold = 1e9;
    long long updates = 0;
    double UPS;
    SDL_Event event;

    const Uint8 *keyboard_state;
    double target_ups = 600.0;
    long long target_update_ns = static_cast<long long>(1e9 / target_ups);
    double world_timer = 0.0;


    player_obj *shield;
    shield = reinterpret_cast<player_obj *>(malloc(sizeof(player_obj_s)));
    if (shield == nullptr) {
        printf("unable to init shield");
        exit(-1);
    }
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution color(0, 0xffffff);

//    unsigned int shield_radius = (window_dims->height > window_dims->width) ? window_dims->width / 4 :
//                                 window_dims->height / 4;
    unsigned int shield_radius = 20;
    float shield_radial_speed = M_PI * 4;
//    shield->color = color(rng) << 8 | 0xff;
    shield->color = 719700991;
    shield->body = new SDL_FRect{
            (float) game_state->window_dims->width / 4.0f,
            (float) game_state->window_dims->height / 4.0f,
            4.0,
            4.0
    };
    shield->pos = new player_pos{
            &shield->body->x,
            &shield->body->y
    };
    shield->id = last_entity_id++;
    printf("shield color: %u\n", shield->color);

    PLAYER_VELOCITY = (float) (PLAYER_VELOCITY * 60 * 1e2 / (double) target_update_ns);
    PLAYER_TARGET_VELOCITY = PLAYER_VELOCITY;
    connect_to_server(player);
    last_update = std::chrono::high_resolution_clock::now();

    const int TRACES_POINTS = 10000;
    SDL_FRect trace[3 * TRACES_POINTS]{};
    int last_trace_id = 0;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    while (running) {
        while (SDL_PollEvent(&event) != 0) {
            switch (event.type) {
                case SDL_EVENT_KEY_UP:
                case SDL_EVENT_KEY_DOWN:
                    network_used = 1;
                    break;
            }
        }

        keyboard_state = SDL_GetKeyboardState(nullptr);
        if (keyboard_state[SDL_SCANCODE_ESCAPE]) {
            running = false;
            disconnect_from_server(player);
        }

        if (keyboard_state[SDL_SCANCODE_DOWN] || keyboard_state[SDL_SCANCODE_S]) {
            event_handlers.keydown(SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_DOWN), game_state);
        }

        if (keyboard_state[SDL_SCANCODE_LEFT] || keyboard_state[SDL_SCANCODE_A]) {
            event_handlers.keydown(SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LEFT), game_state);
        }

        if (keyboard_state[SDL_SCANCODE_RIGHT] || keyboard_state[SDL_SCANCODE_D]) {
            event_handlers.keydown(SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RIGHT), game_state);
        }

        if (keyboard_state[SDL_SCANCODE_UP] || keyboard_state[SDL_SCANCODE_W]) {
            event_handlers.keydown(SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_UP), game_state);
        }

        if (keyboard_state[SDL_SCANCODE_R]) {
            event_handlers.keydown(SDL_SCANCODE_R, game_state);
        }

        if (keyboard_state[SDL_SCANCODE_LSHIFT]) {
            event_handlers.keydown(SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LSHIFT), game_state);
        } else {
            event_handlers.keyup(SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LSHIFT), game_state);
        }

        if (keyboard_state[SDL_SCANCODE_LCTRL]) {
            event_handlers.keydown(SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LCTRL), game_state);
        } else {
            event_handlers.keyup(SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LCTRL), game_state);
        }

//        *player->pos->x = initX + static_cast<float> (100 * cos(world_timer));
//        *player->pos->y = initY + static_cast<float> (100 * sin(world_timer));

        *shield->pos->x =
                player->body->x + player->body->w / 2 +
                static_cast<float> (shield_radius * cos(world_timer * shield_radial_speed * 2));
        *shield->pos->y =
                player->body->y + player->body->h / 2 +
                static_cast<float> (shield_radius * sin(world_timer * shield_radial_speed * 2));

        trace[last_trace_id++] = {
                *player->pos->x + player->body->w / 2,
                *player->pos->y + player->body->h / 2,
                1.0,
                1.0
        };
        trace[last_trace_id++] = {
                *shield->pos->x,
                *shield->pos->y,
                1.0,
                1.0
        };

        if (last_trace_id > (TRACES_POINTS - 3)) {
            last_trace_id -= 3;
            memmove(trace, trace + 3, (last_trace_id) * sizeof(SDL_FRect));
        }

        if (network_used) {
            send_message_with_client_only(network_client);
            purge_network_buff();
            network_used = 0;
        }

        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, player->color);
        SDL_RenderFillRect(renderer, player->body);
        SDL_SetRenderDrawColor(renderer, shield->color);
        SDL_RenderFillRect(renderer, shield->body);

        for (int i = 0; i < 2 * std::min(TRACES_POINTS, last_trace_id); i += 2) {
            SDL_SetRenderDrawColor(
                    renderer,
                    (player->color >> 8) << 8 | (0xff * i / std::min(TRACES_POINTS, last_trace_id))
            );
            SDL_RenderFillRect(
                    renderer,
                    &trace[i]
            );
            SDL_SetRenderDrawColor(
                    renderer,
                    (shield->color >> 8) << 8 | (0xff * i / std::min(TRACES_POINTS, last_trace_id))
            );
            SDL_RenderFillRect(
                    renderer,
                    &trace[i + 1]
            );
        }

        SDL_SetRenderDrawColor(renderer, BACKGROUND);
        SDL_RenderPresent(renderer);

        now = std::chrono::high_resolution_clock::now();

        diff = now - last_update;
        time_counter += diff.count();
        updates++;

        if (time_counter >= time_counter_threshold - target_update_ns) {
            UPS = (double) updates * 1e9 / (double) time_counter;
//            printf(
//                    "ups=%f/s, timer_counter=%llds, updates=%lld, target_sleep_time=%lldms, diff=%lldms\n",
//                    UPS,
//                    time_counter,
//                    updates,
//                    std::max(std::max((target_update_ns) - diff.count(), 0ll), target_update_ns),
//                    diff.count()
//            );
            updates = 0;
            time_counter = 0.0;
        }

//        usleep(
//                (useconds_t) std::max(
//                        std::max(
//                                (long long) ((double) (target_update_ns - diff.count()) / 1e3),
//                                0ll
//                        ),
//                        (long long) ((double) target_update_ns / 1e3)
//                )
//        );
        world_timer += (double) target_update_ns / 1e9;
        last_update = now;
    }
    free(shield);
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

SDL_Renderer *
create_renderer() {

}

SDL_Window *
create_window() {

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
run_window() {
    struct server_info_s *server_params;
    int argc = 4;
    char const *argv[] = {nullptr, "inet", "127.0.0.1", "10312"};
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

//    if (argc == 1) {
//        printf("usage: %s <unix|local> [descriptor path] or <inet> [host] [port]\n", argv[0]);
//        exit(-1);
//    }

    fill_server_info(server_params, argc, const_cast<char **>(argv));

    puts("initing SDL..");
    if (SDL_Init(SDL_INIT_VIDEO)) {
        cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return;
    }
    puts("done");
    auto *game_state = static_cast<client_game_state *>(malloc(sizeof(client_game_state)));
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

//    player->color = color(rng) << 8 | 0xff;
    player->color = 12900112 << 8 | 0xff;
    player->body = new SDL_FRect{
//            initX(rng),
//            initY(rng),
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
    char const *username_raw = "Veritaris";
//    calloc_safe(char *, username_raw, 64, sizeof(char));
//    std::cout << "Enter username:" << std::endl;
//    std::cin >> username_raw;
    player->username = read_into_unicode_string(username_raw);
    puts("player constructed");
    printf("Logged in as: %s\n", compress_into_bytes_array(player->username)->data);
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
//            0
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
    puts("network client constructed");

    main_loop(renderer, game_state);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    free(network_client);
    SDL_Quit();
}

int
main([[maybe_unused]] int argc, char **argv) {
    run_window();
}
