#pragma once

#include <cstdint>

#include <SDL3/SDL.h>

#include "ppu.hpp"


/// @brief SDL3 wrapper class for window and graphics.
class Display
{
public:
    /// @brief Initializes SDL resources
    /// @param ppu Provides PPU frame buffer data.
    /// @throws `std::runtime_error` If any SDL resources fail to initialize.
    Display(Ppu& ppu);

    /// @brief Frees all SDL resources.
    ~Display();

    /// @brief Polls SDL events.
    void handle_events();

    /// @brief Updates screen using PPU's current frame buffer.
    void update_screen();
    void update_screen(const uint32_t* frame_buffer); // Mostly for testing

    /* Getters & Setters */
    inline bool is_program_running() { return is_running; }

private:
    Ppu& ppu;

    /* SDL resources */
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    SDL_Event event;

    bool is_running = true;
};
