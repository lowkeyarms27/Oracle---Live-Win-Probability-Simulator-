#pragma once
#include <SDL2/SDL.h>
#include <cstdint>

constexpr int SCALE = 10; // 640x320 window

class Display {
public:
    Display() = default;
    ~Display();

    bool init(const char* title);
    void render(const uint8_t* framebuffer);
    void clear();
    void destroy();

private:
    SDL_Window*   window{};
    SDL_Renderer* renderer{};
    SDL_Texture*  texture{};
};
