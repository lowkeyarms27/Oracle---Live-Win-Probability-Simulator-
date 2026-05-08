#include <SDL2/SDL.h>
#include <iostream>
#include "chip8.hpp"
#include "display.hpp"
#include "input.hpp"
#include "audio.hpp"
#include "rom.hpp"

constexpr int CYCLES_PER_FRAME = 10;
constexpr int TARGET_FPS       = 60;
constexpr int FRAME_DELAY_MS   = 1000 / TARGET_FPS;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: chip8-emu <rom_path>\n";
        return 1;
    }

    if (!ROM::validate(argv[1])) {
        std::cerr << "Invalid or oversized ROM: " << argv[1] << '\n';
        return 1;
    }

    Chip8   cpu;
    Display display;
    Input   input;
    Audio   audio;

    cpu.initialize();
    if (!cpu.loadROM(argv[1])) {
        std::cerr << "Failed to load ROM\n";
        return 1;
    }

    if (!display.init("CHIP-8 Emulator")) {
        std::cerr << "Failed to init display\n";
        return 1;
    }

    audio.init();

    bool running = true;
    SDL_Event event;

    while (running) {
        uint32_t frameStart = SDL_GetTicks();

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN) input.mapKeyDown(event.key.keysym.sym, cpu.keys);
            if (event.type == SDL_KEYUP)   input.mapKeyUp(event.key.keysym.sym, cpu.keys);
        }

        for (int i = 0; i < CYCLES_PER_FRAME; ++i)
            cpu.emulateCycle();

        if (cpu.halted) running = false;

        cpu.updateTimers();
        audio.beep(cpu.soundTimer > 0);

        if (cpu.drawFlag) {
            display.render(cpu.display, cpu.getDisplayWidth(), cpu.getDisplayHeight());
            cpu.drawFlag = false;
        }

        uint32_t elapsed = SDL_GetTicks() - frameStart;
        if (elapsed < FRAME_DELAY_MS)
            SDL_Delay(FRAME_DELAY_MS - elapsed);
    }

    return 0;
}
