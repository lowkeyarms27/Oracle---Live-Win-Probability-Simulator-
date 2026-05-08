#pragma once
#include <SDL2/SDL.h>
#include <cstdint>

// Chip-8 hex keypad layout:
//  1 2 3 C        mapped to        1 2 3 4
//  4 5 6 D                         Q W E R
//  7 8 9 E                         A S D F
//  A 0 B F                         Z X C V

class Input {
public:
    void mapKeyDown(SDL_Keycode key, uint8_t* keys);
    void mapKeyUp(SDL_Keycode key, uint8_t* keys);
};
