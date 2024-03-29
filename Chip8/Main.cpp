#include <iostream>
#include <chrono>
#include <thread>
#include "stdint.h"
#include <SDL.h>
#include "Chip8.h"

uint8_t keyMap[16] = {
    SDLK_x, 
    SDLK_1, SDLK_2, SDLK_3,
    SDLK_q, SDLK_w, SDLK_e,
    SDLK_a, SDLK_s, SDLK_d,
    SDLK_z, SDLK_c, SDLK_4,
    SDLK_r, SDLK_f, SDLK_v
};

int main(int argc, char** argv) {

    if (argc != 2) {
        std::cout << "Usage: chip8 <ROM>" << std::endl;
        return 1;
    }

    Chip8 chip8 = Chip8();

    int w = 960;                   // Window width 64*15
    int h = 480;                   // Window height 32*15

    // The window we'll be rendering to
    SDL_Window* window = NULL;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }
    // Create window
    window = SDL_CreateWindow("CHIP8 Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        exit(2);
    }

    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_RenderSetLogicalSize(renderer, w, h);

    // Create texture that stores frame buffer
    SDL_Texture* sdlTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);

    // Temporary pixel buffer
    uint32_t pixels[2048];

load:
    if (!chip8.Load(argv[1]))
        return 1;

    // Emulation loop
    while (true) {
        chip8.Cycle();

        // Process SDL events
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) exit(0);

            // Process keydown events
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE)
                    exit(0);

                if (e.key.keysym.sym == SDLK_F1)
                    goto load;

                for (int i = 0; i < 16; ++i) {
                    if (e.key.keysym.sym == keyMap[i]) {
                        chip8.Key[i] = 1;
                    }
                }
            }
            // Process keyup events
            if (e.type == SDL_KEYUP) {
                for (int i = 0; i < 16; ++i) {
                    if (e.key.keysym.sym == keyMap[i]) {
                        chip8.Key[i] = 0;
                    }
                }
            }
        }

        // If draw occurred, redraw SDL screen
        if (chip8.drawFlag) {
            chip8.drawFlag = false;

            // Store pixels in temporary buffer
            for (int i = 0; i < 2048; ++i) {
                uint8_t pixel = chip8.GFX[i];
                pixels[i] = (0x00FFFFFF * pixel) | 0xFF000000;
            }
            // Update SDL texture
            SDL_UpdateTexture(sdlTexture, NULL, pixels, 64 * sizeof(Uint32));
            // Clear screen and render
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, sdlTexture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }

}