#include "display.hpp"

#include <stdexcept>
#include <iostream>

Display::Display(Ppu& _ppu) : 
    ppu(_ppu)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) 
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        throw std::runtime_error("Failed Initializing SDL");
    }

    if (!SDL_CreateWindowAndRenderer(
        "Game Boy Colour Emulator", 
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

void Display::handle_events()
{
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            is_running = false;
            break;

        case SDL_EVENT_WINDOW_RESIZED:
            {
                constexpr float ASPECT_RATIO = GBResolution::WIDTH / GBResolution::HEIGHT;
                int window_width, window_height;
                SDL_GetWindowSize(window, &window_width, &window_height);
                std::cout << "Window Width: " << std::dec << window_width << " Window Height: " << window_height << '\n';
                // int new_height = window_width / ASPECT_RATIO;
                // SDL_SetRenderLogicalPresentation(renderer, window_width, new_height, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
            }
            break;

        case SDL_EVENT_KEY_DOWN:
            switch(event.key.scancode)
            {
            case SDL_SCANCODE_ESCAPE:
                is_running = false;
                break;
            case SDL_SCANCODE_1:
                SDL_SetWindowSize(window, GBResolution::WIDTH, GBResolution::HEIGHT);
                break;
            case SDL_SCANCODE_2:
                SDL_SetWindowSize(window, GBResolution::WIDTH * 2, GBResolution::HEIGHT * 2);
                break;
            case SDL_SCANCODE_3:
                SDL_SetWindowSize(window, GBResolution::WIDTH * 3, GBResolution::HEIGHT * 3);
                break;
            case SDL_SCANCODE_4:
                SDL_SetWindowSize(window, GBResolution::WIDTH * 4, GBResolution::HEIGHT * 4);
                break;
            }

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