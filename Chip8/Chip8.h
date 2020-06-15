#pragma once

class Chip8 {
private:
    uint16_t Stack[16];                 // Stack
    uint16_t SP;                        // Stack pointer

    uint8_t Memory[4096];               // Memory (4k)
    uint8_t V[16];                      // V registers (V0-VF)

    uint16_t PC;                        // Program counter
    uint16_t opCode;                    // Current op code
    uint16_t I;                         // Index

    uint8_t delay_timer;                // Delay timer
    uint8_t sound_timer;                // Sound timer

    void Init();
    void Draw(uint8_t X, uint8_t Y, uint8_t N);
    void ClearScreen();

public:
    uint8_t  GFX[64 * 32];              // Graphics buffer
    uint8_t  Key[16];                   // Keypad
    bool drawFlag;

    Chip8();
    ~Chip8();

    void Cycle();
    bool Load(const char* file); 
};