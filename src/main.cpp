//
// Created by Георгий Имешкенов on 08.12.2023.
//

#include <iostream>

#define SDL_MAIN_HANDLED

#include <SDL.h>
#include "client.c"

using std::cerr, std::cin, std::cout;
using Clock = std::chrono::high_resolution_clock;
using TimeStamp = std::chrono::time_point<Clock>;

const unsigned int RED_MASK = 0xff000000;
const unsigned int GREEN_MASK = 0x00ff0000;
const unsigned int BLUE_MASK = 0x0000ff00;
const unsigned int ALPHA_MASK = 0x000000ff;

bool running = true;
struct client_s *network_client;
unsigned char NETWORKBUFF[MAXNETWORKBUFFSIZE] = {0};

typedef void (*quit_handler_t)(void);

typedef void (*keydown_handler_t)(SDL_Keycode);

typedef void (*keyup_handler_t)(SDL_Keycode);

struct {
    quit_handler_t quit;
    keydown_handler_t keydown;
    keyup_handler_t keyup;
} event_handlers;

size_t NETWORK_BUFFER_FILL = NETWORK_BUFFER_OFFSET;

size_t
write_to_network_buff(const char *data) {
    size_t data_len = strlen(data);
    size_t rem_len = data_len;

    if (NETWORK_BUFFER_FILL == MAXNETWORKBUFFSIZE) return data_len;

    for (size_t i = NETWORK_BUFFER_FILL; i < MAXNETWORKBUFFSIZE && rem_len > 0; i++, rem_len--) {
        *(NETWORKBUFF + i) = *(data + i - NETWORK_BUFFER_OFFSET);
    }

    NETWORK_BUFFER_FILL = (rem_len > 0) ? MAXNETWORKBUFFSIZE : NETWORK_BUFFER_FILL + data_len;
//    printf(
//            "wrote %zu bytes of data to NETWORKBUFF, %zu bytes remain, %zu bytes total\n",
//            data_len - rem_len,
//            rem_len,
//            data_len
//    );

    return rem_len;
}

void
handle_keyboard(SDL_Keycode keycode) {
    switch (keycode) {
        case SDLK_ESCAPE: {
            running = false;
//            printf("exiting game\n");
            write_to_network_buff("exit");
            break;
        }
        case SDLK_UP:
        case SDLK_w:
            write_to_network_buff("up");
            break;
        case SDLK_DOWN:
        case SDLK_s:
            write_to_network_buff("down");
            break;
        case SDLK_LEFT:
        case SDLK_a:
            write_to_network_buff("left");
            break;
        case SDLK_RIGHT:
        case SDLK_d:
            write_to_network_buff("right");
            break;
        case SDLK_SPACE:
            write_to_network_buff("jump");
            break;
        default:
            printf("%d\n", keycode);
            break;
    }
}

void
purge_network_buff() {
    memset(NETWORKBUFF + NETWORK_BUFFER_OFFSET, '\0', MAXNETWORKBUFFSIZE - NETWORK_BUFFER_OFFSET);
    NETWORK_BUFFER_FILL = NETWORK_BUFFER_OFFSET;
}

void
main_loop(SDL_Renderer *renderer) {
    int network_used = 0;
    event_handlers.keydown = handle_keyboard;

    while (running) {
        SDL_Event event;
        if (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYDOWN:
                    event_handlers.keydown(event.key.keysym.sym);
                    network_used = 1;
                    break;
            }

            if (network_used) {
                send_message(network_client, (char *) NETWORKBUFF);
                purge_network_buff();
                network_used = 0;
            }
        }
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }
}

void
SDL_SetRenderDrawColor(SDL_Renderer *renderer, unsigned int color) {
    SDL_SetRenderDrawColor(
            renderer,
            (color & RED_MASK) >> 24,
            (color & GREEN_MASK) >> 16,
            (color & BLUE_MASK) >> 8,
            color & ALPHA_MASK);
}

void
run_window() {
    struct server_info_s *server_params;
    int argc = 4;
    char const *argv[] = {nullptr, "inet", "127.0.0.1", "10312"};

    server_params = static_cast<server_info_s *>(malloc(server_info_size));
    if (server_params == nullptr) {
        allocwarn("struct server_info_s");
        exit(-1);
    }

    if (argc == 1) {
        printf("usage: %s <unix|local> [descriptor path] or <inet> [host] [port]\n", argv[0]);
        exit(-1);
    }

    fill_server_info(server_params, argc, const_cast<char **>(argv));

    SDL_SetMainReady();

    if (SDL_Init(SDL_INIT_VIDEO)) {
        cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_DisplayMode displayMode;
    SDL_GetDesktopDisplayMode(0, &displayMode);
    int width = displayMode.w / 4;
    int height = displayMode.h / 4;

    if (SDL_CreateWindowAndRenderer(
            width,
            height,
            SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS,
            &window,
            &renderer)
            ) {
        cerr << "Failed to create winded and renderer: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_SetWindowTitle(window, "gengine test");
    SDL_SetRenderDrawColor(renderer, 0xffffffff);

    network_client = create_client(server_params);
    if (network_client == nullptr) {
        std::cout << "unable to init network client, exiting" << std::endl;
        exit(-1);
    }

    main_loop(renderer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    free(network_client);
    SDL_Quit();
}

int
main(int argc, char **argv) {
    run_window();
}
