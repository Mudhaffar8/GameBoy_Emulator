#pragma once

#include <cstdint>

#include <SDL3/SDL.h>

#include "ppu.hpp"

/// @brief SDL3 wrapper class for graphics and input handling.
class Display
{
public:
    /// @brief Initializes SDL resources
    /// @param ppu Provides PPU frame buffer data.
    /// @throws `std::runtime_error` If any SDL resources fail to initialize.
    Display(Ppu& ppu);

    /// @brief Frees all SDL resources.
    ~Display();

    /// @brief Polls SDL input events and updates input buffer.
    /// @param buffer Input buffer to be updated.
    void handle_input(std::array<bool, 16>& buffer);

    /// @brief Updates screen using PPU's current frame buffer.
    void update_screen();
    void update_screen(const uint32_t* frame_buffer); // Mostly for testing

    /* Getters & Setters */
    inline bool is_program_running() { return is_running; }
    inline void set_scale_factor(int scale_factor) { this->scale_factor = scale_factor; }

private:
    Ppu& ppu;

    /* SDL resources */
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    SDL_Event event;

    bool is_running = true;
    int scale_factor = 1;
};
