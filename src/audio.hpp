#pragma once
#include <SDL2/SDL.h>

class Audio {
public:
    Audio() = default;
    ~Audio();

    bool init();
    void beep(bool on);
    void destroy();

private:
    SDL_AudioDeviceID deviceId{};
    bool playing{};

    static void audioCallback(void* userdata, uint8_t* stream, int len);
};
