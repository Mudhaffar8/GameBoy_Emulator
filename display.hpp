#pragma once

#include <cstdint>

#include <SDL3/SDL.h>
#include <SDL3/SDL_keyboard.h>

#include "ppu.hpp"

namespace ColourSpecs
{
static const uint32_t COLOUR00 = 0x9BBC0FFF;
static const uint32_t COLOUR01 = 0X8BAC0FFF;
static const uint32_t COLOUR10 = 0x306230FF;
static const uint32_t COLOUR11 = 0x0F380FFF;
}

class Display
{
public:
    Display();
    ~Display();

    void handle_input(bool* buffer);
    void update_screen(const uint8_t* buffer);

    inline bool is_program_running() { return is_running; }
    inline void set_scale_factor(int scale_factor) { this->scale_factor = scale_factor; }

private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    SDL_Event event;

    bool is_running = true;
    int scale_factor = 1;
};
