#include "display.hpp"
#include "chip8.hpp"
#include <cstring>

Display::~Display() { destroy(); }

bool Display::init(const char* title) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return false;

    window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        DISPLAY_W * SCALE, DISPLAY_H * SCALE,
        SDL_WINDOW_SHOWN
    );
    if (!window) return false;

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return false;

    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        DISPLAY_W, DISPLAY_H
    );
    return texture != nullptr;
}

void Display::render(const uint8_t* framebuffer) {
    uint32_t pixels[DISPLAY_W * DISPLAY_H];
    for (int i = 0; i < DISPLAY_W * DISPLAY_H; ++i)
        pixels[i] = framebuffer[i] ? 0xFFFFFFFF : 0x000000FF;

    SDL_UpdateTexture(texture, nullptr, pixels, DISPLAY_W * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void Display::clear() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

void Display::destroy() {
    if (texture)  SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window)   SDL_DestroyWindow(window);
    SDL_Quit();
    texture  = nullptr;
    renderer = nullptr;
    window   = nullptr;
}
