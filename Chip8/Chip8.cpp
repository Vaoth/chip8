#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <random>
#include "time.h"
#include "chip8.h"

unsigned char chip8_fontset[80] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, //0
    0x20, 0x60, 0x20, 0x20, 0x70, //1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
    0x90, 0x90, 0xF0, 0x10, 0x10, //4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
    0xF0, 0x10, 0x20, 0x40, 0x40, //7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
    0xF0, 0x90, 0xF0, 0x90, 0x90, //A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
    0xF0, 0x80, 0x80, 0x80, 0xF0, //C
    0xE0, 0x90, 0x90, 0x90, 0xE0, //D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
    0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};

Chip8::Chip8() {
    Init();
}
Chip8::~Chip8() {}

// Initialise
void Chip8::Init() {
    PC = 0x200;    // Set program counter to 0x200
    opCode = 0;        // Reset op code
    I = 0;          // Reset I
    SP = 0;        // Reset Stack pointer

    // Clear the Stack, Keypad, and V registers
    for (uint8_t i = 0; i < 16; ++i) {
        Stack[i] = 0;
        Key[i] = 0;
        V[i] = 0;
    }

    // Clear Memory
    for (uint16_t i = 0; i < 4096; ++i) {
        Memory[i] = 0;
    }

    // Load font set into Memory
    for (uint8_t i = 0; i < 80; ++i) {
        Memory[i] = chip8_fontset[i];
    }

    ClearScreen();

    // Reset timers
    delay_timer = 0;
    sound_timer = 0;

    // Seed rng
    srand(time(NULL));
}

// Initialise and load ROM into Memory
bool Chip8::Load(const char* file) {
    // Initialise
    Init();

    printf("Loading ROM: %s\n", file);

    // Open ROM file
    FILE* rom;
    fopen_s(&rom, file, "rb");
    if (rom == NULL) {
        std::cerr << "Failed to open ROM" << std::endl;
        return false;
    }

    // Get file size
    fseek(rom, 0, SEEK_END);
    long rom_size = ftell(rom);
    rewind(rom);

    // Allocate Memory to store rom
    char* rom_buffer = (char*)malloc(sizeof(char) * rom_size);
    if (rom_buffer == NULL) {
        std::cerr << "Failed to allocate Memory for ROM" << std::endl;
        return false;
    }

    // Copy ROM into buffer
    size_t result = fread(rom_buffer, sizeof(char), (size_t)rom_size, rom);
    if (result != rom_size) {
        std::cerr << "Failed to read ROM" << std::endl;
        return false;
    }

    // Copy buffer to Memory
    if ((4096 - 512) > rom_size) {
        for (int i = 0; i < rom_size; ++i) {
            Memory[i + 512] = (uint8_t)rom_buffer[i];   // Load into Memory starting
                                                        // at 0x200 (=512)
        }
    }
    else {
        std::cerr << "ROM too large to fit in Memory" << std::endl;
        return false;
    }

    // Clean up
    fclose(rom);
    free(rom_buffer);

    return true;
}

// Emulate one cycle
void Chip8::Cycle() {

    // Fetch op code
    opCode = Memory[PC] << 8 | Memory[PC + 1];   // Op code is two bytes
    printf("%02x %02x|", Memory[PC], Memory[PC + 1]);

    unsigned char X = (opCode & 0x0F00) >> 8;
    unsigned char Y = (opCode & 0x00F0) >> 4;
    unsigned char N = (opCode & 0x000F);
    unsigned char NN = opCode & 0x00FF;
    unsigned short NNN = opCode & 0x0FFF;

    switch (opCode & 0xF000) {
    case 0x0000:
        switch (opCode & 0x000F) { 
        case 0x0000: //0x00E0: Clear screen
            ClearScreen();
            break;

        case 0x000E: //0x00EE: Return from subroutine
            PC = Stack[--SP];
            break;

        default:
            printf("Unknown opCode: 0x%X\n", opCode);
        }
        break;

    case 0x1000: //0x1NNN: Jumps to address NNN
        PC = NNN;
        return;

    case 0x2000: //0x2NNN: Calls subroutine at NNN
        Stack[SP++] = PC;
        PC = NNN;
        return;

    case 0x3000: //0x3XNN: Skips the next instruction if VX equals NN
        PC += V[X] == NN ? 2 : 0;
        break;

    case 0x4000: //0x4XNN: Skips the next instruction if VX does not equal NN
        PC += V[X] != NN  ? 2 : 0;
        break;

    case 0x5000: //0x5XY0: Skips the next instruction if VX equals VY
        PC += V[X] == V[Y] ? 2 : 0;
        break;

    case 0x6000: //0x6XNN: Sets VX to NN
        V[X] = NN;
        break;

    case 0x7000: //0x7XNN: Adds NN to VX
        V[X] += NN;
        break;

    case 0x8000:
        switch (opCode & 0x000F) {
        case 0x0000: //0x8XY0: Sets VX to the value of VY
            V[X] = V[Y];
            break;

        case 0x0001: //0x8XY1: Sets VX to (VX OR VY)
            V[X] |= V[Y];
            break;

            
        case 0x0002: //0x8XY2: Sets VX to (VX AND VY)
            V[X] &= V[Y];
            break;

            
        case 0x0003: //0x8XY3: Sets VX to (VX XOR VY)
            V[X] ^= V[Y];
            break;

        case 0x0004: //0x8XY4: Adds VY to VX, VF is the carry
            V[X] += V[Y];
            V[0xF] = V[Y] > (0xFF - V[X]);
            break;

        case 0x0005: //0x8XY5: VY is subtracted from VX, VF is the borrow
            V[0xF] = V[Y] > V[X];
            V[X] -= V[Y];
            break;

        case 0x0006: //0x8XY6: Shifts VX right by one, VF is the least significant bit of VX before the shift
            V[0xF] = V[X] & 0x1;
            V[X] >>= 1;
            break;

        case 0x0007: // 0x8XY7: Sets VX to VY minus VX. VF is ~borrow
            V[0xF] = V[X] > V[Y];
            V[X] = V[Y] - V[X];
            break;

            
        case 0x000E: //0x8XYE: Shifts VX left by one, VF is the most significant bit of VX before the shift
            V[0xF] = V[X] >> 7;
            V[X] <<= 1;
            break;

        default:
            printf("Unknown opCode: 0x%X\n", opCode);
        }
        break;

    case 0x9000: //0x9XY0: Skips the next instruction if VX doesn't equal VY
        PC += V[X] != V[Y] ? 2 : 0;
        break;

    case 0xA000: //0xANNN: Sets I to the address NNN.
        I = NNN;
        break;

        
    case 0xB000: //0xBNNN: Jumps to the address NNN plus V0
        PC = (NNN) + V[0];
        break;

    case 0xC000: //0xCXNN: Sets VX to a random number, masked by NN
        V[X] = (rand() % (0xFF + 1)) & (NN);
        break;

    case 0xD000: { //0xDXYN: Draw
        Draw(X, Y, N);
    }
    break;

    case 0xE000:
        switch (NN) {
        case 0x009E: //0xEX9E: Skips the next instruction if the Key stored in VX is pressed
            PC += Key[V[X]] != 0 ? 2 : 0;
            break;
            
        case 0x00A1: //0xEXA1: Skips the next instruction if the Key stored in VX isn't pressed
            PC += Key[V[X]] == 0 ? 2 : 0;
            break;

        default:
            printf("Unknown opCode: 0x%X\n", opCode);
        }
        break;

    case 0xF000:
        switch (NN) {
        case 0x0007: //0xFX07: VX is set to the value of the delay timer
            V[X] = delay_timer;
            break;

            
        case 0x000A: { //0xFX0A: A Key press is awaited, and then stored in VX
            bool keyPressed = false;

            for (uint8_t i = 0; i < 16; ++i) {
                if (Key[i] != 0) {
                    V[X] = i;
                    keyPressed = true;
                }
            }

            if (!keyPressed)
                return;
        }
        break;

        
        case 0x0015: //0xFX15: Sets the delay timer to VX
            delay_timer = V[X];
            break;

            
        case 0x0018: //0xFX18: Sets the sound timer to VX
            sound_timer = V[X];
            break;

            
        case 0x001E: //0xFX1E: Adds VX to I, VF is set to 1 when range overflow
            V[0xF] = I + V[X] > 0xFFF;
            I += V[X];
            break;

        case 0x0029: //0xFX29: Sets I to the location of the SPrite for the character in VX. Characters 0-F (in hexadecimal) are represented by a 4x5 font
            I = V[X] * 0x5;
            break;

        case 0x0033: //0xFX33: Stores the Binary-coded decimal representation of VX
            Memory[I] = V[X] / 100;
            Memory[I + 1] = (V[X] / 10) % 10;
            Memory[I + 2] = V[X] % 10;
            break;
            
        case 0x0055: //0xFX55: Stores V0 to VX in Memory starting at address I
            for (uint8_t i = 0; i <= (X); ++i)
                Memory[I + i] = V[i];

            I += (X) + 1;
            break;

        case 0x0065: //0xFX65: Loads V0 to VX from Memory starting at address I
            for (uint8_t i = 0; i <= (X); ++i)
                V[i] = Memory[I + i];

            I += (X) + 1;
            break;

        default:
            printf("Unknown opCode: 0x%X\n", opCode);
        }
        break;

    default:
        printf("\nUnknown opCode: %.4X\n", opCode);
    }
    PC += 2;

    if (delay_timer > 0)
        --delay_timer;

    if (sound_timer > 0)
        if (sound_timer == 1)
            //beep
    --sound_timer;
}

void Chip8::Draw(uint8_t X, uint8_t Y, uint8_t N) {
    uint16_t x = V[X];
    uint16_t y = V[Y];
    uint16_t height = N;
    uint16_t pixel;

    V[0xF] = 0;
    for (uint8_t yline = 0; yline < height; yline++) {
        pixel = Memory[I + yline];
        for (uint8_t xline = 0; xline < 8; xline++) {
            if ((pixel & (0x80 >> xline)) != 0) {
                if (GFX[(x + xline + ((y + yline) * 64))] == 1)
                    V[0xF] = 1;
                GFX[x + xline + ((y + yline) * 64)] ^= 1;
            }
        }
    }

    drawFlag = true;
}

void Chip8::ClearScreen() {
    for (uint16_t i = 0; i < 2048; ++i) {
        GFX[i] = 0;
    }
    drawFlag = true;
}