#pragma once
#include <SDL2/SDL.h>
#include <cstdint>

constexpr int SCALE = 10; // 640x320 window

class Display {
public:
    Display() = default;
    ~Display();

    bool init(const char* title);
    void render(const uint8_t* framebuffer, int width, int height);
    void clear();
    void destroy();

private:
    bool ensureTexture(int width, int height);

    SDL_Window*   window{};
    SDL_Renderer* renderer{};
    SDL_Texture*  texture{};
    int textureW{};
    int textureH{};
};
