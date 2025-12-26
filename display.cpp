#include <stdexcept>

#include "display.hpp"
#include <iostream>

#include <SDL3/SDL_keyboard.h>

Display::Display(Ppu& _ppu) : 
    ppu(_ppu)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) 
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        throw std::runtime_error("Failed Initializing SDL");
    }

    if (!SDL_CreateWindowAndRenderer(
        "CHIP-8 Emulator", 
        GBResolution::WIDTH, 
        GBResolution::HEIGHT, 
        SDL_WINDOW_RESIZABLE, 
        &window, &renderer
    )) 
    {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        throw std::runtime_error("Creating window/renderer failed");
    }

    texture = SDL_CreateTexture(
        renderer, 
        SDL_PIXELFORMAT_RGBA8888, 
        SDL_TEXTUREACCESS_STREAMING, 
        GBResolution::WIDTH, 
        GBResolution::HEIGHT
    );

    if (!texture)
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw std::runtime_error("Could not initialize texture");
    }
}

Display::~Display()
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
}

void Display::handle_input(bool* key_input)
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
            }
            break;
        
        case SDL_EVENT_KEY_UP:
            break;
        
        default:
            break;
        }
    }
}

void Display::update_screen()
{
    int pitch = 0;
    uint32_t* pixels = nullptr;

    SDL_LockTexture(texture, nullptr, (void**)(&pixels), &pitch);

    std::copy(ppu.frame_buffer.begin(), ppu.frame_buffer.end(), pixels);

    SDL_UnlockTexture(texture);

    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void Display::update_screen(const uint32_t* frame_buffer)
{
    int pitch = 0;
    uint32_t* pixels = nullptr;

    SDL_LockTexture(texture, nullptr, (void**)(&pixels), &pitch);

    std::copy(frame_buffer, frame_buffer + GBResolution::DIMENSIONS, pixels);

    SDL_UnlockTexture(texture);

    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}