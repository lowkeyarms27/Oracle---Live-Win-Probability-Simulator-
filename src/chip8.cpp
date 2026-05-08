#include "chip8.hpp"
#include <fstream>
#include <cstring>
#include <stdexcept>

static constexpr uint8_t FONT_SET[80] = {
    0xF0,0x90,0x90,0x90,0xF0, // 0
    0x20,0x60,0x20,0x20,0x70, // 1
    0xF0,0x10,0xF0,0x80,0xF0, // 2
    0xF0,0x10,0xF0,0x10,0xF0, // 3
    0x90,0x90,0xF0,0x10,0x10, // 4
    0xF0,0x80,0xF0,0x10,0xF0, // 5
    0xF0,0x80,0xF0,0x90,0xF0, // 6
    0xF0,0x10,0x20,0x40,0x40, // 7
    0xF0,0x90,0xF0,0x90,0xF0, // 8
    0xF0,0x90,0xF0,0x10,0xF0, // 9
    0xF0,0x90,0xF0,0x90,0x90, // A
    0xE0,0x90,0xE0,0x90,0xE0, // B
    0xF0,0x80,0x80,0x80,0xF0, // C
    0xE0,0x90,0x90,0x90,0xE0, // D
    0xF0,0x80,0xF0,0x80,0xF0, // E
    0xF0,0x80,0xF0,0x80,0x80  // F
};

void Chip8::initialize() {
    PC = ROM_START;
    I  = 0;
    SP = 0;
    drawFlag = false;
    std::memset(memory,  0, sizeof(memory));
    std::memset(V,       0, sizeof(V));
    std::memset(stack,   0, sizeof(stack));
    std::memset(display, 0, sizeof(display));
    std::memset(keys,    0, sizeof(keys));
    delayTimer = 0;
    soundTimer = 0;
    loadFont();
}

void Chip8::loadFont() {
    std::memcpy(memory + FONT_START, FONT_SET, sizeof(FONT_SET));
}

bool Chip8::loadROM(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return false;

    std::streamsize size = file.tellg();
    if (size > static_cast<std::streamsize>(MEMORY_SIZE - ROM_START)) return false;

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(memory + ROM_START), size);
    return file.good();
}

void Chip8::emulateCycle() {
    uint16_t opcode = (memory[PC] << 8) | memory[PC + 1];
    PC += 2;
    executeOpcode(opcode);
}

void Chip8::updateTimers() {
    if (delayTimer > 0) --delayTimer;
    if (soundTimer > 0) --soundTimer;
}

void Chip8::executeOpcode(uint16_t opcode) {
    // TODO: implement all 35 opcodes — agent fills this in
    (void)opcode;
}
