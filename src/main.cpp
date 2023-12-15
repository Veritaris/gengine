//
// Created by Георгий Имешкенов on 08.12.2023.
//

#include <iostream>

#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <sys/socket.h>

using std::cerr, std::cin, std::cout;
using Clock = std::chrono::high_resolution_clock;
using TimeStamp = std::chrono::time_point<Clock>;

const unsigned int RED_MASK = 0xff000000;
const unsigned int GREEN_MASK = 0x00ff0000;
const unsigned int BLUE_MASK = 0x0000ff00;
const unsigned int ALPHA_MASK = 0x000000ff;

void main_loop(SDL_Renderer *renderer) {
    while (true) {
        SDL_Event event;
        if (
                SDL_PollEvent(&event) &&
                (SDL_QUIT == event.type || (SDL_KEYDOWN == event.type && SDLK_ESCAPE == event.key.keysym.sym))
                ) {
            break;
        }
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }
}

void SDL_SetRenderDrawColor(SDL_Renderer *renderer, unsigned int color) {
    SDL_SetRenderDrawColor(
            renderer,
            (color & RED_MASK) >> 24,
            (color & GREEN_MASK) >> 16,
            (color & BLUE_MASK) >> 8,
            color & ALPHA_MASK);
}

void run_window() {
    SDL_SetMainReady();

    if (SDL_Init(SDL_INIT_VIDEO)) {
        cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_DisplayMode displayMode;
    SDL_GetDesktopDisplayMode(0, &displayMode);
    int width = displayMode.w;
    int height = displayMode.h;

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

    main_loop(renderer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main() {

}
