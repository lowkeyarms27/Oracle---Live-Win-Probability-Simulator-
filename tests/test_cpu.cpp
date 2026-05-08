#include <catch2/catch_test_macros.hpp>
#include "chip8.hpp"

TEST_CASE("Chip8 initializes to clean state", "[cpu]") {
    Chip8 cpu;
    cpu.initialize();

    REQUIRE(cpu.PC == ROM_START);
    REQUIRE(cpu.I  == 0);
    REQUIRE(cpu.SP == 0);
    REQUIRE(cpu.drawFlag == false);

    for (int i = 0; i < NUM_REGISTERS; ++i)
        REQUIRE(cpu.V[i] == 0);
}

TEST_CASE("Font is loaded into memory at FONT_START", "[cpu]") {
    Chip8 cpu;
    cpu.initialize();
    // Font byte 0 of '0' sprite is 0xF0
    REQUIRE(cpu.memory[FONT_START] == 0xF0);
}

TEST_CASE("Timers decrement each update", "[cpu]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.delayTimer = 5;
    cpu.soundTimer = 3;
    cpu.updateTimers();
    REQUIRE(cpu.delayTimer == 4);
    REQUIRE(cpu.soundTimer == 2);
}

TEST_CASE("Timers clamp at zero", "[cpu]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.delayTimer = 0;
    cpu.updateTimers();
    REQUIRE(cpu.delayTimer == 0);
}

// TODO: opcode tests will be added by CPU agent
