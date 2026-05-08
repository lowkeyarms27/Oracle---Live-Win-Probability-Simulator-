#include <catch2/catch_test_macros.hpp>
#include "chip8.hpp"

// Helper: write a 2-byte opcode at ROM_START and run one cycle
static void runOpcode(Chip8& cpu, uint16_t opcode) {
    cpu.memory[ROM_START]     = static_cast<uint8_t>(opcode >> 8);
    cpu.memory[ROM_START + 1] = static_cast<uint8_t>(opcode & 0xFF);
    cpu.PC = ROM_START;
    cpu.emulateCycle();
}

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

// ── 6XKK: LD Vx, byte ────────────────────────────────────────────────────────
TEST_CASE("6XKK: LD sets register correctly", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    // 6A42 => V[A] = 0x42
    runOpcode(cpu, 0x6A42);
    REQUIRE(cpu.V[0xA] == 0x42);
}

TEST_CASE("6XKK: LD sets register V0", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    runOpcode(cpu, 0x60FF);
    REQUIRE(cpu.V[0] == 0xFF);
}

// ── 7XKK: ADD Vx, byte (no carry) ────────────────────────────────────────────
TEST_CASE("7XKK: ADD accumulates value", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[2] = 0x10;
    runOpcode(cpu, 0x7205); // V[2] += 5
    REQUIRE(cpu.V[2] == 0x15);
}

TEST_CASE("7XKK: ADD wraps without setting carry", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[1] = 0xFF;
    runOpcode(cpu, 0x7101); // V[1] += 1, wraps to 0
    REQUIRE(cpu.V[1] == 0x00);
    REQUIRE(cpu.V[0xF] == 0); // no carry flag set
}

// ── 8XY4: ADD Vx, Vy with carry ──────────────────────────────────────────────
TEST_CASE("8XY4: ADD sets carry flag on overflow", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[0] = 0xFF;
    cpu.V[1] = 0x01;
    runOpcode(cpu, 0x8014); // V[0] += V[1]
    REQUIRE(cpu.V[0xF] == 1);
    REQUIRE(cpu.V[0] == 0x00);
}

TEST_CASE("8XY4: ADD clears carry flag when no overflow", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[0] = 0x10;
    cpu.V[1] = 0x05;
    cpu.V[0xF] = 1; // pre-set to ensure it gets cleared
    runOpcode(cpu, 0x8014);
    REQUIRE(cpu.V[0xF] == 0);
    REQUIRE(cpu.V[0] == 0x15);
}

// ── 8XY5: SUB Vx, Vy ─────────────────────────────────────────────────────────
TEST_CASE("8XY5: SUB sets VF=1 when Vx > Vy (no borrow)", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[0] = 0x10;
    cpu.V[1] = 0x05;
    runOpcode(cpu, 0x8015); // V[0] -= V[1]
    REQUIRE(cpu.V[0xF] == 1);
    REQUIRE(cpu.V[0] == 0x0B);
}

TEST_CASE("8XY5: SUB sets VF=0 when Vx < Vy (borrow)", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[0] = 0x05;
    cpu.V[1] = 0x10;
    runOpcode(cpu, 0x8015); // V[0] -= V[1], underflow
    REQUIRE(cpu.V[0xF] == 0);
}

TEST_CASE("8XY5: SUB sets VF=1 when Vx == Vy (no borrow)", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[0] = 0x2A;
    cpu.V[1] = 0x2A;
    runOpcode(cpu, 0x8015);
    REQUIRE(cpu.V[0] == 0x00);
    REQUIRE(cpu.V[0xF] == 1);
}

// ── 1NNN: JP addr ─────────────────────────────────────────────────────────────
TEST_CASE("1NNN: JP sets PC correctly", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    runOpcode(cpu, 0x1300); // JP 0x300
    REQUIRE(cpu.PC == 0x300);
}

TEST_CASE("1NNN: JP sets PC to any valid address", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    runOpcode(cpu, 0x1ABC);
    REQUIRE(cpu.PC == 0xABC);
}

// ── 2NNN / 00EE: CALL / RET ──────────────────────────────────────────────────
TEST_CASE("2NNN: CALL pushes PC and jumps", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    // After fetch, PC has advanced to ROM_START+2; CALL stores that
    runOpcode(cpu, 0x2400); // CALL 0x400
    REQUIRE(cpu.PC == 0x400);
    REQUIRE(cpu.SP == 1);
    REQUIRE(cpu.stack[0] == ROM_START + 2);
}

TEST_CASE("00EE: RET pops PC from stack", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    // Simulate a prior CALL: push a return address onto the stack manually
    cpu.stack[0] = 0x300;
    cpu.SP = 1;
    runOpcode(cpu, 0x00EE);
    REQUIRE(cpu.PC == 0x300);
    REQUIRE(cpu.SP == 0);
}

TEST_CASE("00FE/00FF: SCHIP mode switches display resolution", "[cpu][opcodes][schip]") {
    Chip8 cpu;
    cpu.initialize();

    runOpcode(cpu, 0x00FF); // high-res
    REQUIRE(cpu.highRes == true);
    REQUIRE(cpu.getDisplayWidth() == DISPLAY_W_HI);
    REQUIRE(cpu.getDisplayHeight() == DISPLAY_H_HI);

    runOpcode(cpu, 0x00FE); // low-res
    REQUIRE(cpu.highRes == false);
    REQUIRE(cpu.getDisplayWidth() == DISPLAY_W);
    REQUIRE(cpu.getDisplayHeight() == DISPLAY_H);
}

TEST_CASE("00FD: SCHIP exit halts execution", "[cpu][opcodes][schip]") {
    Chip8 cpu;
    cpu.initialize();
    runOpcode(cpu, 0x00FD);
    REQUIRE(cpu.halted == true);
}

TEST_CASE("2NNN then 00EE: CALL/RET round-trip", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    uint16_t callSite = ROM_START + 2; // PC after fetch

    runOpcode(cpu, 0x2500); // CALL 0x500
    REQUIRE(cpu.PC == 0x500);

    // Now place RET at PC (0x500) and run it
    runOpcode(cpu, 0x00EE);
    REQUIRE(cpu.PC == callSite);
    REQUIRE(cpu.SP == 0);
}

// ── 3XKK: SE Vx, byte ────────────────────────────────────────────────────────
TEST_CASE("3XKK: SE skips next instruction when equal", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[3] = 0x42;
    runOpcode(cpu, 0x3342); // SE V3, 0x42
    REQUIRE(cpu.PC == ROM_START + 4); // skipped one instruction
}

TEST_CASE("3XKK: SE does not skip when not equal", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[3] = 0x10;
    runOpcode(cpu, 0x3342);
    REQUIRE(cpu.PC == ROM_START + 2);
}

// ── 4XKK: SNE Vx, byte ───────────────────────────────────────────────────────
TEST_CASE("4XKK: SNE skips when not equal", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[5] = 0x10;
    runOpcode(cpu, 0x4542); // SNE V5, 0x42
    REQUIRE(cpu.PC == ROM_START + 4);
}

TEST_CASE("4XKK: SNE does not skip when equal", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[5] = 0x42;
    runOpcode(cpu, 0x4542);
    REQUIRE(cpu.PC == ROM_START + 2);
}

// ── ANNN: LD I, addr ─────────────────────────────────────────────────────────
TEST_CASE("ANNN: LD I sets I correctly", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    runOpcode(cpu, 0xA123);
    REQUIRE(cpu.I == 0x123);
}

TEST_CASE("ANNN: LD I sets I to max address", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    runOpcode(cpu, 0xAFFF);
    REQUIRE(cpu.I == 0xFFF);
}

// ── DXYN: DRW — drawFlag ─────────────────────────────────────────────────────
TEST_CASE("DXYN: drawFlag is set after draw opcode", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[0] = 0;
    cpu.V[1] = 0;
    cpu.I = FONT_START; // point at a font sprite
    cpu.drawFlag = false;
    runOpcode(cpu, 0xD011); // DRW V0, V1, 1
    REQUIRE(cpu.drawFlag == true);
}

TEST_CASE("DXYN: collision detection sets VF", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    // Place a pixel manually
    cpu.display[0] = 1;
    cpu.V[0] = 0;
    cpu.V[1] = 0;
    cpu.I = FONT_START; // 0xF0 — has bit 7 set, will hit display[0]
    runOpcode(cpu, 0xD011);
    REQUIRE(cpu.V[0xF] == 1);
}

TEST_CASE("DXYN: no collision leaves VF=0", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    // display is all zero; drawing into it should not collide
    cpu.V[0] = 0;
    cpu.V[1] = 0;
    cpu.I = FONT_START;
    runOpcode(cpu, 0xD011);
    REQUIRE(cpu.V[0xF] == 0);
}

TEST_CASE("DXY0 in high-res draws 16x16 sprite", "[cpu][opcodes][schip]") {
    Chip8 cpu;
    cpu.initialize();
    runOpcode(cpu, 0x00FF); // high-res mode

    cpu.V[0] = 0;
    cpu.V[1] = 0;
    cpu.I = 0x300;

    // 16 rows of 0x8000: left-most pixel set each row
    for (int r = 0; r < 16; ++r) {
        cpu.memory[0x300 + r * 2] = 0x80;
        cpu.memory[0x300 + r * 2 + 1] = 0x00;
    }

    runOpcode(cpu, 0xD010); // DXY0
    for (int r = 0; r < 16; ++r)
        REQUIRE(cpu.display[r * DISPLAY_W_HI] == 1);
}

// ── FX33: BCD ────────────────────────────────────────────────────────────────
TEST_CASE("FX33: BCD encodes 156 as [1, 5, 6]", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[2] = 156;
    cpu.I = 0x300;
    runOpcode(cpu, 0xF233);
    REQUIRE(cpu.memory[0x300] == 1);
    REQUIRE(cpu.memory[0x301] == 5);
    REQUIRE(cpu.memory[0x302] == 6);
}

TEST_CASE("FX33: BCD encodes 255 as [2, 5, 5]", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[0] = 255;
    cpu.I = 0x300;
    runOpcode(cpu, 0xF033);
    REQUIRE(cpu.memory[0x300] == 2);
    REQUIRE(cpu.memory[0x301] == 5);
    REQUIRE(cpu.memory[0x302] == 5);
}

TEST_CASE("FX33: BCD encodes 0 as [0, 0, 0]", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[1] = 0;
    cpu.I = 0x300;
    runOpcode(cpu, 0xF133);
    REQUIRE(cpu.memory[0x300] == 0);
    REQUIRE(cpu.memory[0x301] == 0);
    REQUIRE(cpu.memory[0x302] == 0);
}

// ── FX55 / FX65: store / load registers ──────────────────────────────────────
TEST_CASE("FX55/FX65: round-trip store then load", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    // Set registers V0-V3
    cpu.V[0] = 0xAA;
    cpu.V[1] = 0xBB;
    cpu.V[2] = 0xCC;
    cpu.V[3] = 0xDD;
    cpu.I = 0x300;

    // Store V0-V3 (FX55 with X=3)
    runOpcode(cpu, 0xF355);
    REQUIRE(cpu.memory[0x300] == 0xAA);
    REQUIRE(cpu.memory[0x301] == 0xBB);
    REQUIRE(cpu.memory[0x302] == 0xCC);
    REQUIRE(cpu.memory[0x303] == 0xDD);

    // Wipe registers
    cpu.V[0] = 0; cpu.V[1] = 0; cpu.V[2] = 0; cpu.V[3] = 0;
    cpu.I = 0x300;

    // Load V0-V3 (FX65 with X=3)
    runOpcode(cpu, 0xF365);
    REQUIRE(cpu.V[0] == 0xAA);
    REQUIRE(cpu.V[1] == 0xBB);
    REQUIRE(cpu.V[2] == 0xCC);
    REQUIRE(cpu.V[3] == 0xDD);
}

// ── 00E0: CLS ─────────────────────────────────────────────────────────────────
TEST_CASE("00E0: CLS clears display and sets drawFlag", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    // Set some pixels
    cpu.display[0] = 1;
    cpu.display[100] = 1;
    cpu.drawFlag = false;
    runOpcode(cpu, 0x00E0);
    REQUIRE(cpu.drawFlag == true);
    for (int i = 0; i < DISPLAY_W * DISPLAY_H; ++i)
        REQUIRE(cpu.display[i] == 0);
}

// ── 5XY0: SE Vx, Vy ──────────────────────────────────────────────────────────
TEST_CASE("5XY0: SE Vx Vy skips when equal", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[2] = 0x55;
    cpu.V[4] = 0x55;
    runOpcode(cpu, 0x5240);
    REQUIRE(cpu.PC == ROM_START + 4);
}

TEST_CASE("5XY0: SE Vx Vy does not skip when not equal", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[2] = 0x55;
    cpu.V[4] = 0x44;
    runOpcode(cpu, 0x5240);
    REQUIRE(cpu.PC == ROM_START + 2);
}

// ── 8XY0: LD Vx, Vy ──────────────────────────────────────────────────────────
TEST_CASE("8XY0: LD copies Vy into Vx", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[3] = 0x77;
    cpu.V[5] = 0x00;
    runOpcode(cpu, 0x8530); // V[5] = V[3]
    REQUIRE(cpu.V[5] == 0x77);
}

// ── 8XY1/2/3: bitwise ops ─────────────────────────────────────────────────────
TEST_CASE("8XY1: OR combines registers", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[0] = 0xF0;
    cpu.V[1] = 0x0F;
    runOpcode(cpu, 0x8011);
    REQUIRE(cpu.V[0] == 0xFF);
}

TEST_CASE("8XY2: AND masks registers", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[0] = 0xFF;
    cpu.V[1] = 0x0F;
    runOpcode(cpu, 0x8012);
    REQUIRE(cpu.V[0] == 0x0F);
}

TEST_CASE("8XY3: XOR toggles bits", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[0] = 0xFF;
    cpu.V[1] = 0xF0;
    runOpcode(cpu, 0x8013);
    REQUIRE(cpu.V[0] == 0x0F);
}

// ── 8XY6 / 8XYE: shifts ──────────────────────────────────────────────────────
TEST_CASE("8XY6: SHR shifts right and stores LSB in VF", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[2] = 0x05; // 0000 0101, LSB = 1
    runOpcode(cpu, 0x8206);
    REQUIRE(cpu.V[2] == 0x02);
    REQUIRE(cpu.V[0xF] == 1);
}

TEST_CASE("8XYE: SHL shifts left and stores MSB in VF", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[2] = 0x81; // 1000 0001, MSB = 1
    runOpcode(cpu, 0x820E);
    REQUIRE(cpu.V[2] == 0x02);
    REQUIRE(cpu.V[0xF] == 1);
}

// ── 8XY7: SUBN ───────────────────────────────────────────────────────────────
TEST_CASE("8XY7: SUBN sets VF=1 when Vy > Vx", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[0] = 0x05;
    cpu.V[1] = 0x10;
    runOpcode(cpu, 0x8017); // V[0] = V[1] - V[0]
    REQUIRE(cpu.V[0xF] == 1);
    REQUIRE(cpu.V[0] == 0x0B);
}

// ── 9XY0: SNE Vx, Vy ─────────────────────────────────────────────────────────
TEST_CASE("9XY0: SNE Vx Vy skips when not equal", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[0] = 0x01;
    cpu.V[1] = 0x02;
    runOpcode(cpu, 0x9010);
    REQUIRE(cpu.PC == ROM_START + 4);
}

// ── BNNN: JP V0, addr ─────────────────────────────────────────────────────────
TEST_CASE("BNNN: JP V0 adds V0 to address", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[0] = 0x10;
    runOpcode(cpu, 0xB300); // PC = V[0] + 0x300
    REQUIRE(cpu.PC == 0x310);
}

// ── FX07 / FX15 / FX18: timer ops ────────────────────────────────────────────
TEST_CASE("FX07: V[X] = delayTimer", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.delayTimer = 42;
    runOpcode(cpu, 0xF007);
    REQUIRE(cpu.V[0] == 42);
}

TEST_CASE("FX15: delayTimer = V[X]", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[1] = 30;
    runOpcode(cpu, 0xF115);
    REQUIRE(cpu.delayTimer == 30);
}

TEST_CASE("FX18: soundTimer = V[X]", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[2] = 15;
    runOpcode(cpu, 0xF218);
    REQUIRE(cpu.soundTimer == 15);
}

// ── FX1E: I += V[X] ──────────────────────────────────────────────────────────
TEST_CASE("FX1E: I += V[X]", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.I = 0x200;
    cpu.V[3] = 0x10;
    runOpcode(cpu, 0xF31E);
    REQUIRE(cpu.I == 0x210);
}

// ── FX29: I = font address for V[X] ──────────────────────────────────────────
TEST_CASE("FX29: I points to font sprite for digit", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[0] = 0x5; // digit 5
    runOpcode(cpu, 0xF029);
    REQUIRE(cpu.I == FONT_START + 5 * 5);
}

TEST_CASE("FX30: I points to large SCHIP font sprite for digit", "[cpu][opcodes][schip]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[0] = 0xA;
    runOpcode(cpu, 0xF030);
    REQUIRE(cpu.I == FONT_START + 80 + 10 * 10);
}

TEST_CASE("00CN: SCHIP scroll down by N lines", "[cpu][opcodes][schip]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.display[0] = 1; // top-left
    runOpcode(cpu, 0x00C2); // down 2
    REQUIRE(cpu.display[0] == 0);
    REQUIRE(cpu.display[2 * DISPLAY_W] == 1);
}

TEST_CASE("00FB/00FC: SCHIP horizontal scroll", "[cpu][opcodes][schip]") {
    Chip8 cpu;
    cpu.initialize();

    cpu.display[0] = 1;
    runOpcode(cpu, 0x00FB); // right 4
    REQUIRE(cpu.display[0] == 0);
    REQUIRE(cpu.display[4] == 1);

    runOpcode(cpu, 0x00FC); // left 4
    REQUIRE(cpu.display[4] == 0);
    REQUIRE(cpu.display[0] == 1);
}

// ── EX9E / EXA1: key skip ─────────────────────────────────────────────────────
TEST_CASE("EX9E: SKP skips when key is pressed", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[0] = 3;
    cpu.keys[3] = 1;
    runOpcode(cpu, 0xE09E);
    REQUIRE(cpu.PC == ROM_START + 4);
}

TEST_CASE("EX9E: SKP does not skip when key not pressed", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[0] = 3;
    cpu.keys[3] = 0;
    runOpcode(cpu, 0xE09E);
    REQUIRE(cpu.PC == ROM_START + 2);
}

TEST_CASE("EXA1: SKNP skips when key is not pressed", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.V[0] = 5;
    cpu.keys[5] = 0;
    runOpcode(cpu, 0xE0A1);
    REQUIRE(cpu.PC == ROM_START + 4);
}

// ── FX0A: wait for key ────────────────────────────────────────────────────────
TEST_CASE("FX0A: stalls (re-executes) when no key pressed", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    // No keys pressed
    runOpcode(cpu, 0xF00A);
    // PC should be back at ROM_START (re-executed)
    REQUIRE(cpu.PC == ROM_START);
}

TEST_CASE("FX0A: stores key index when key is pressed", "[cpu][opcodes]") {
    Chip8 cpu;
    cpu.initialize();
    cpu.keys[7] = 1;
    runOpcode(cpu, 0xF00A);
    REQUIRE(cpu.V[0] == 7);
    REQUIRE(cpu.PC == ROM_START + 2);
}
