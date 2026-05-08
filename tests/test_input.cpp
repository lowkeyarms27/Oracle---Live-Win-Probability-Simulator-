#include <catch2/catch_test_macros.hpp>
#include "chip8.hpp"
#include "input.hpp"

TEST_CASE("Key X maps to chip8 key 0x0", "[input]") {
    Input input;
    uint8_t keys[NUM_KEYS]{};
    input.mapKeyDown(SDLK_x, keys);
    REQUIRE(keys[0x0] == 1);
}

TEST_CASE("Key release clears key state", "[input]") {
    Input input;
    uint8_t keys[NUM_KEYS]{};
    input.mapKeyDown(SDLK_x, keys);
    input.mapKeyUp(SDLK_x, keys);
    REQUIRE(keys[0x0] == 0);
}

TEST_CASE("Unknown key is ignored", "[input]") {
    Input input;
    uint8_t keys[NUM_KEYS]{};
    input.mapKeyDown(SDLK_ESCAPE, keys);
    for (int i = 0; i < NUM_KEYS; ++i)
        REQUIRE(keys[i] == 0);
}
