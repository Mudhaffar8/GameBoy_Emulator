#pragma once

#include <cstdint>

#include <SDL3/SDL.h>

#include "ppu.hpp"

class Display
{
    friend class Ppu;

public:
    Display(Ppu* ppu);
    ~Display();

    void handle_input(bool* buffer);

    void update_screen();
    void update_screen(const uint32_t* frame_buffer);

    inline bool is_program_running() { return is_running; }
    inline void set_scale_factor(int scale_factor) { this->scale_factor = scale_factor; }

private:
    Ppu* ppu;

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    SDL_Event event;

    bool is_running = true;
    int scale_factor = 1;
};
