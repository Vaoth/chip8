#include <iostream>

const unsigned short START_MEMORY_LOCATION = 0x200;
const unsigned short START_SCREEN_LOCATION = 0xF00;

class Chip8 {
    bool Active = false;

    unsigned char Memory[4096];
    unsigned char V[16];                // Registers
    unsigned char GFX[64 * 32];         // Screen
    unsigned char Key[16];              // Keypad

    unsigned short opCode;
    unsigned short I;
    unsigned short PC;                  // Program counter

    unsigned short Stack[16];
    unsigned short SP;                  // Stack pointer

    unsigned char delayTimer;
    unsigned char soundTimer;

    void Init() {
        PC = START_MEMORY_LOCATION;
        opCode = 0;
        I = 0;
        PC = 0;
        SP = 0;
    }

    void Cycle() {
        opCode = Memory[PC] << 8 | Memory[PC + 1];

        unsigned char X = (opCode & 0x0F00) >> 8;
        unsigned char Y = (opCode & 0x00F0) >> 4;
        unsigned char N = (opCode & 0x000F);
        unsigned char NN = opCode & 0x00FF;
        unsigned short NNN = opCode & 0x0FFF;

        switch (opCode & 0xF000) {
        case 0x0000:
            switch (opCode & 0x000F) {
            case 0x0000:
                DisplayClear();
                break;
            case 0x000E:

                break;
            }
            break;

        case 0x1000:
            PC = opCode & 0x0FFF;
            return;

        case 0x2000:
            Stack[SP++] = PC;
            PC = opCode & 0x0FFF;
            return;

        case 0x3000:
            if (V[X] == NN)
                PC += 2;
            break;

        case 0x4000:
            if (V[X] != NN)
                PC += 2;
            break;

        case 0x5000:
            if (V[X] == V[Y])
                PC += 2;
            break;

        case 0x6000:
            V[X] == NN;
            break;

        case 0x7000:
            V[X] += NN;
            break;

        case 0x8000:
            switch (opCode & 0x000F) {
                V[X] = V[Y];
                break;

            case 0x0001:
                V[X] |= V[Y];
                break;

            case 0x0002:
                V[X] &= V[Y];
                break;

            case 0x0003:
                V[X] ^= V[Y];
                break;

            case 0x0004:
                unsigned char temp = V[X];
                V[X] += V[Y];
                V[15] = temp > V[X];
                break;

            case 0x0005:
                unsigned char temp = V[X];
                V[X] -= V[Y];
                V[15] = temp < V[X];
                break;

            case 0x0006:
                V[15] = V[X] & 1;
                V[X] >>= 1;
                break;

            case 0x0007:
                unsigned char temp = V[Y];
                V[X] = V[Y] - V[X];
                V[15] = temp < V[X];
                break;

            case 0x000E:
                V[15] = (V[X] >> 7) & 1;
                V[X] <<= 1;
                break;
            }
            break;

        case 0x9000:
            if (V[X] != V[Y])
                PC += 2;
            break;

        case 0xA000:
            I = NNN;
            break;

        case 0xB000:
            PC = V[0] + NNN;
            break;

        case 0xC000:
            V[X] = ((unsigned char)rand()) & NN;
            break;

        case 0xD000:
            Draw(&X, &Y, &N);
            break;

        case 0xE000:
            switch (opCode & 0x00FF) {
            case 0x009E:
                if (Key[V[X]] != 0)
                    PC += 2;
                break;
            case 0x00A1:
                if (Key[V[X]] == 0)
                    PC += 2;
                break;
            }
            break;

        case 0xF000:
            switch (opCode & 0x00FF) {
            case 0x0007:
                V[X] = delayTimer;
                break;
            case 0x000A:
                V[X] = GetKey();
                break;
            case 0x0015:
                delayTimer = V[X];
                break;
            case 0x0018:
                soundTimer = V[X];
                break;
            case 0x001E:
                V[15] = I + V[X] > 0xFFF;
                I += V[X];
                break;
            case 0x0029:
                I = chip8_fontset[V[X]];
                break;
            case 0x0033:
                Memory[I] = V[X] / 100;
                Memory[I + 1] = (V[X] / 10) % 10;
                Memory[I + 2] = (V[X] % 100) % 10;
                break;
            case 0x0055:
                Dump(&X);
                break;
            case 0x0065:
                Load(&X);
                break;
            }
            break;
        }
        PC += 2;
    }

    void DisplayClear();
    void Draw(unsigned char* X, unsigned char* Y, unsigned char* N);
    void Dump(unsigned char* X);
    void Load(unsigned char* X);
    unsigned short GetKey();

    unsigned char chip8_fontset[80] =
    {
      0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
      0x20, 0x60, 0x20, 0x20, 0x70, // 1
      0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
      0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
      0x90, 0x90, 0xF0, 0x10, 0x10, // 4
      0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
      0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
      0xF0, 0x10, 0x20, 0x40, 0x40, // 7
      0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
      0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
      0xF0, 0x90, 0xF0, 0x90, 0x90, // A
      0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
      0xF0, 0x80, 0x80, 0x80, 0xF0, // C
      0xE0, 0x90, 0x90, 0x90, 0xE0, // D
      0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
      0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };
};