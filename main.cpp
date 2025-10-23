#include <SDL3/SDL.h>
#include <SDL3/SDL_keyboard.h>

#include <iostream>

#include "cpu.hpp"
#include "ppu.hpp"
#include "display.hpp"

int main(int argc, char** argv)
{
    uint8_t buffer[GBResolution::HEIGHT * GBResolution::WIDTH]{};
    std::fill(buffer, buffer + sizeof(buffer), 1);

    Display display;
    display.update_screen(buffer);

    SDL_Delay(3500);

    return 0;
}

