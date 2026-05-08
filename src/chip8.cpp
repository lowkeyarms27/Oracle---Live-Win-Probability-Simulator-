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

static constexpr uint8_t FONT_SET_LARGE[160] = {
    0x3C,0x66,0xC3,0xC3,0xC3,0xC3,0xC3,0xC3,0x66,0x3C, // 0
    0x18,0x38,0x78,0x18,0x18,0x18,0x18,0x18,0x18,0x7E, // 1
    0x3C,0x66,0xC3,0x03,0x06,0x0C,0x18,0x30,0x60,0xFF, // 2
    0x3C,0x66,0xC3,0x03,0x1E,0x03,0x03,0xC3,0x66,0x3C, // 3
    0x06,0x0E,0x1E,0x36,0x66,0xC6,0xFF,0x06,0x06,0x06, // 4
    0xFF,0xC0,0xC0,0xFC,0x06,0x03,0x03,0xC3,0x66,0x3C, // 5
    0x3C,0x66,0xC0,0xC0,0xFC,0xC6,0xC3,0xC3,0x66,0x3C, // 6
    0xFF,0x03,0x06,0x0C,0x18,0x18,0x18,0x18,0x18,0x18, // 7
    0x3C,0x66,0xC3,0x66,0x3C,0x66,0xC3,0xC3,0x66,0x3C, // 8
    0x3C,0x66,0xC3,0xC3,0x67,0x3F,0x03,0x03,0x66,0x3C, // 9
    0x18,0x3C,0x66,0xC3,0xC3,0xFF,0xC3,0xC3,0xC3,0xC3, // A
    0xFC,0x66,0x63,0x63,0x7E,0x63,0x63,0x63,0x66,0xFC, // B
    0x3C,0x66,0xC3,0xC0,0xC0,0xC0,0xC0,0xC3,0x66,0x3C, // C
    0xF8,0x6C,0x66,0x63,0x63,0x63,0x63,0x66,0x6C,0xF8, // D
    0xFF,0x60,0x60,0x60,0x7C,0x60,0x60,0x60,0x60,0xFF, // E
    0xFF,0x60,0x60,0x60,0x7C,0x60,0x60,0x60,0x60,0x60  // F
};

static constexpr uint16_t FONT_LARGE_START = FONT_START + sizeof(FONT_SET);

void Chip8::initialize() {
    PC = ROM_START;
    I  = 0;
    SP = 0;
    drawFlag = false;
    highRes = false;
    halted = false;
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
    std::memcpy(memory + FONT_LARGE_START, FONT_SET_LARGE, sizeof(FONT_SET_LARGE));
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
    if (halted) return;
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
        } else if (opcode == 0x00FD) {
            // SCHIP: EXIT
            halted = true;
        } else if (opcode == 0x00FE) {
            // SCHIP: LOW-RES MODE
            highRes = false;
            drawFlag = true;
        } else if (opcode == 0x00FF) {
            // SCHIP: HIGH-RES MODE
            highRes = true;
            drawFlag = true;
        } else if (opcode == 0x00FB) {
            // SCHIP: SCROLL RIGHT 4
            const int w = getDisplayWidth();
            const int h = getDisplayHeight();
            for (int y = 0; y < h; ++y) {
                for (int x = w - 1; x >= 0; --x) {
                    int dst = y * w + x;
                    int srcX = x - 4;
                    display[dst] = (srcX >= 0) ? display[y * w + srcX] : 0;
                }
            }
            drawFlag = true;
        } else if (opcode == 0x00FC) {
            // SCHIP: SCROLL LEFT 4
            const int w = getDisplayWidth();
            const int h = getDisplayHeight();
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    int dst = y * w + x;
                    int srcX = x + 4;
                    display[dst] = (srcX < w) ? display[y * w + srcX] : 0;
                }
            }
            drawFlag = true;
        } else if ((opcode & 0xFFF0) == 0x00C0) {
            // SCHIP: SCROLL DOWN N
            const int n = N;
            const int w = getDisplayWidth();
            const int h = getDisplayHeight();
            for (int y = h - 1; y >= 0; --y) {
                for (int x = 0; x < w; ++x) {
                    int dst = y * w + x;
                    int srcY = y - n;
                    display[dst] = (srcY >= 0) ? display[srcY * w + x] : 0;
                }
            }
            drawFlag = true;
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
            uint8_t borrow = (V[X] >= V[Y]) ? 1 : 0;
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
        const int w = getDisplayWidth();
        const int h = getDisplayHeight();
        uint8_t xPos = V[X] % w;
        uint8_t yPos = V[Y] % h;
        V[0xF] = 0;

        auto drawPixel = [&](int px, int py) {
            int idx = py * w + px;
            if (display[idx]) V[0xF] = 1;
            display[idx] ^= 1;
        };

        if (highRes && N == 0) {
            // SCHIP: 16x16 sprite, two bytes per row.
            for (int row = 0; row < 16; ++row) {
                uint16_t spriteRow = (static_cast<uint16_t>(memory[I + row * 2]) << 8) | memory[I + row * 2 + 1];
                for (int col = 0; col < 16; ++col) {
                    if (spriteRow & (0x8000 >> col)) {
                        int px = (xPos + col) % w;
                        int py = (yPos + row) % h;
                        drawPixel(px, py);
                    }
                }
            }
        } else {
            for (int row = 0; row < N; ++row) {
                uint8_t spriteByte = memory[I + row];
                for (int col = 0; col < 8; ++col) {
                    if (spriteByte & (0x80 >> col)) {
                        int px = (xPos + col) % w;
                        int py = (yPos + row) % h;
                        drawPixel(px, py);
                    }
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
        case 0x30:
            I = FONT_LARGE_START + V[X] * 10;
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

int Chip8::getDisplayWidth() const {
    return highRes ? DISPLAY_W_HI : DISPLAY_W;
}

int Chip8::getDisplayHeight() const {
    return highRes ? DISPLAY_H_HI : DISPLAY_H;
}
