#include <catch2/catch_test_macros.hpp>
#include "chip8.hpp"

// Display tests don't spin up SDL — they validate framebuffer logic only
TEST_CASE("Display dimensions are correct", "[display]") {
    REQUIRE(DISPLAY_W == 64);
    REQUIRE(DISPLAY_H == 32);
}

TEST_CASE("Framebuffer initializes to zero", "[display]") {
    Chip8 cpu;
    cpu.initialize();
    for (int i = 0; i < DISPLAY_PIXELS_MAX; ++i)
        REQUIRE(cpu.display[i] == 0);
}
