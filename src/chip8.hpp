#pragma once
#include <cstdint>
#include <string>

constexpr int DISPLAY_W = 64;
constexpr int DISPLAY_H = 32;
constexpr int DISPLAY_W_HI = 128;
constexpr int DISPLAY_H_HI = 64;
constexpr int DISPLAY_PIXELS_MAX = DISPLAY_W_HI * DISPLAY_H_HI;
constexpr int MEMORY_SIZE = 4096;
constexpr int STACK_DEPTH = 16;
constexpr int NUM_REGISTERS = 16;
constexpr int NUM_KEYS = 16;
constexpr uint16_t ROM_START = 0x200;
constexpr uint16_t FONT_START = 0x050;

class Chip8 {
public:
    uint8_t  memory[MEMORY_SIZE]{};
    uint8_t  V[NUM_REGISTERS]{};    // general-purpose registers V0–VF
    uint16_t I{};                   // index register
    uint16_t PC{};                  // program counter
    uint8_t  SP{};                  // stack pointer
    uint16_t stack[STACK_DEPTH]{};
    uint8_t  delayTimer{};
    uint8_t  soundTimer{};
    uint8_t  display[DISPLAY_PIXELS_MAX]{};
    uint8_t  keys[NUM_KEYS]{};
    bool     drawFlag{};
    bool     highRes{};
    bool     halted{};

    void initialize();
    bool loadROM(const std::string& path);
    void emulateCycle();
    void updateTimers();
    int getDisplayWidth() const;
    int getDisplayHeight() const;

private:
    void loadFont();
    void executeOpcode(uint16_t opcode);
};
