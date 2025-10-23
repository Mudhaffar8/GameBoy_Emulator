#include <stdexcept>

#include "display.hpp"

// #include <SDL3/SDL_render.h>

Display::Display()
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

void Display::update_screen(const uint8_t* buffer)
{
    int pitch = 0;
    uint32_t* pixels = nullptr;

    SDL_LockTexture(texture, NULL, (void**)(&pixels), &pitch);

    for (int y = 0; y < GBResolution::HEIGHT; ++y) 
    {
        for (int x = 0; x < GBResolution::WIDTH; ++x) 
        {   
            int index = y * GBResolution::WIDTH + x;
            int pitch_index = y * (pitch / sizeof(uint32_t)) + x;

            pixels[pitch_index] = buffer[index] == 0 ? ColourSpecs::COLOUR00 : ColourSpecs::COLOUR11; 
        }
    }

    SDL_UnlockTexture(texture);

    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}