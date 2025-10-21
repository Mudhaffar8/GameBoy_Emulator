#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_keyboard.h>

#include <iostream>

#include "ppu.hpp"

int main(int argc, char** argv)
{
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Event event;

    bool is_running = true;
        
    if (!SDL_Init(SDL_INIT_VIDEO)) 
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return -1;
    }

    if (!SDL_CreateWindowAndRenderer("GameBoy Emulator", GAMEBOY_SCREEN_WIDTH, GAMEBOY_SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE , &window, &renderer)) 
    {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return -1;
    }

    SDL_Texture* texture = SDL_CreateTexture(
        renderer, 
        SDL_PIXELFORMAT_RGBA8888, 
        SDL_TEXTUREACCESS_STREAMING, 
        GAMEBOY_SCREEN_WIDTH, 
        GAMEBOY_SCREEN_HEIGHT
    );

    if (!texture)
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    uint32_t frame_buffer[GAMEBOY_SCREEN_HEIGHT * GAMEBOY_SCREEN_WIDTH]{};
    // std::fill(frame_buffer, frame_buffer + (sizeof(frame_buffer) / sizeof(frame_buffer[0])), 0xF000FFFF);

    while (is_running)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                is_running = false;
                break;

            case SDL_EVENT_KEY_DOWN:
                switch(event.key.scancode)
                {
                case SDL_SCANCODE_ESCAPE:
                    is_running = false;
                    break;
                
                case SDL_SCANCODE_J:
                    std::cout << "J Pressed\n";
                    break;
                
                case SDL_SCANCODE_F:
                    std::cout << "F Pressed\n";
                    break;

                default:
                    break;
                }
                break;
            
            case SDL_EVENT_KEY_UP:
                break;
            
            default:
                break;
            }
        }

        int pitch = 0;
        uint32_t* pixels = nullptr;
        SDL_LockTexture(texture, NULL, (void**)(&pixels), &pitch);

        for (int y = 0; y < GAMEBOY_SCREEN_HEIGHT; ++y) 
        {
            for (int x = 0; x < GAMEBOY_SCREEN_WIDTH; ++x) 
            {
                uint8_t r = rand() % 0xFF;
                uint8_t g = rand() % 0xFF;
                uint8_t b = rand() % 0xFF;
                
                int index = y * GAMEBOY_SCREEN_WIDTH + x;

                frame_buffer[index] = (r << 24) | (g << 16) | (b << 8) | 0xFF; // Redundant but necessary for testing
                pixels[y * (pitch / sizeof(uint32_t)) + x] = frame_buffer[index]; 
                
            }
        }

        SDL_UnlockTexture(texture);

        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        SDL_Delay(4);
        // uint64_t before = SDL_GetTicks();
        // uint64_t after = SDL_GetTicks();
        // uint64_t time = after - before;
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    

    SDL_Quit();

    return 0;
}

