#include "chip8.hpp"
#include <fstream>
#include <cstring>
#include <stdexcept>
#include <cstdlib>

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
    uint8_t  nibble = (opcode >> 12) & 0xF;
    uint8_t  X      = (opcode >> 8)  & 0xF;
    uint8_t  Y      = (opcode >> 4)  & 0xF;
    uint8_t  N      = opcode & 0xF;
    uint8_t  KK     = opcode & 0xFF;
    uint16_t NNN    = opcode & 0xFFF;

    switch (nibble) {
    case 0x0:
        if (opcode == 0x00E0) {
            // CLS
            std::memset(display, 0, sizeof(display));
            drawFlag = true;
        } else if (opcode == 0x00EE) {
            // RET
            PC = stack[--SP];
        }
        break;

    case 0x1:
        // JP addr
        PC = NNN;
        break;

    case 0x2:
        // CALL addr
        stack[SP++] = PC;
        PC = NNN;
        break;

    case 0x3:
        // SE Vx, byte
        if (V[X] == KK) PC += 2;
        break;

    case 0x4:
        // SNE Vx, byte
        if (V[X] != KK) PC += 2;
        break;

    case 0x5:
        // SE Vx, Vy
        if (V[X] == V[Y]) PC += 2;
        break;

    case 0x6:
        // LD Vx, byte
        V[X] = KK;
        break;

    case 0x7:
        // ADD Vx, byte (no carry)
        V[X] += KK;
        break;

    case 0x8:
        switch (N) {
        case 0x0: V[X] = V[Y]; break;
        case 0x1: V[X] |= V[Y]; break;
        case 0x2: V[X] &= V[Y]; break;
        case 0x3: V[X] ^= V[Y]; break;
        case 0x4: {
            uint16_t sum = static_cast<uint16_t>(V[X]) + V[Y];
            V[0xF] = (sum > 0xFF) ? 1 : 0;
            V[X] = static_cast<uint8_t>(sum & 0xFF);
            break;
        }
        case 0x5: {
            uint8_t borrow = (V[X] > V[Y]) ? 1 : 0;
            V[X] -= V[Y];
            V[0xF] = borrow;
            break;
        }
        case 0x6: {
            uint8_t lsb = V[X] & 0x1;
            V[X] >>= 1;
            V[0xF] = lsb;
            break;
        }
        case 0x7: {
            uint8_t borrow = (V[Y] > V[X]) ? 1 : 0;
            V[X] = V[Y] - V[X];
            V[0xF] = borrow;
            break;
        }
        case 0xE: {
            uint8_t msb = (V[X] >> 7) & 0x1;
            V[X] <<= 1;
            V[0xF] = msb;
            break;
        }
        }
        break;

    case 0x9:
        // SNE Vx, Vy
        if (V[X] != V[Y]) PC += 2;
        break;

    case 0xA:
        // LD I, addr
        I = NNN;
        break;

    case 0xB:
        // JP V0, addr
        PC = static_cast<uint16_t>(V[0]) + NNN;
        break;

    case 0xC:
        // RND Vx, byte
        V[X] = static_cast<uint8_t>(std::rand() % 256) & KK;
        break;

    case 0xD: {
        // DRW Vx, Vy, nibble
        uint8_t xPos = V[X] % DISPLAY_W;
        uint8_t yPos = V[Y] % DISPLAY_H;
        V[0xF] = 0;
        for (int row = 0; row < N; ++row) {
            uint8_t spriteByte = memory[I + row];
            for (int col = 0; col < 8; ++col) {
                if (spriteByte & (0x80 >> col)) {
                    int px = (xPos + col) % DISPLAY_W;
                    int py = (yPos + row) % DISPLAY_H;
                    int idx = py * DISPLAY_W + px;
                    if (display[idx]) V[0xF] = 1;
                    display[idx] ^= 1;
                }
            }
        }
        drawFlag = true;
        break;
    }

    case 0xE:
        if (KK == 0x9E) {
            // SKP Vx
            if (keys[V[X]]) PC += 2;
        } else if (KK == 0xA1) {
            // SKNP Vx
            if (!keys[V[X]]) PC += 2;
        }
        break;

    case 0xF:
        switch (KK) {
        case 0x07:
            V[X] = delayTimer;
            break;
        case 0x0A: {
            // Wait for key press
            bool keyPressed = false;
            for (int k = 0; k < NUM_KEYS; ++k) {
                if (keys[k]) {
                    V[X] = static_cast<uint8_t>(k);
                    keyPressed = true;
                    break;
                }
            }
            if (!keyPressed) PC -= 2; // re-execute this instruction
            break;
        }
        case 0x15:
            delayTimer = V[X];
            break;
        case 0x18:
            soundTimer = V[X];
            break;
        case 0x1E:
            I += V[X];
            break;
        case 0x29:
            I = FONT_START + V[X] * 5;
            break;
        case 0x33:
            memory[I]     = V[X] / 100;
            memory[I + 1] = (V[X] / 10) % 10;
            memory[I + 2] = V[X] % 10;
            break;
        case 0x55:
            for (int r = 0; r <= X; ++r)
                memory[I + r] = V[r];
            break;
        case 0x65:
            for (int r = 0; r <= X; ++r)
                V[r] = memory[I + r];
            break;
        }
        break;
    }
}
