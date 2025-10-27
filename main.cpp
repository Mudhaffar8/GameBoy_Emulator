#include <SDL3/SDL.h>
#include <SDL3/SDL_keyboard.h>

#include <iostream>
#include <chrono>
#include <cstdint>

#include "cpu.hpp"
#include "ppu.hpp"
#include "memory.hpp"
#include "display.hpp"
#include "cpu.hpp"

void speed_test();
void ppu_test();

int main(int argc, char** argv)
{
    ppu_test();   
    return 0;
}

void ppu_test()
{
    Mmu mmu;
    Ppu ppu(&mmu);
    Display display(&ppu);

    // 0xAA, 0xAA, 0x55, 0x55, 0xAA, 0xAA, 0x55, 0x55, 0xAA, 0xAA, 0x55, 0x55, 0xAA, 0xAA, 0x55, 0x55,
    for (int j = 0; j < 0x1800; j++)
    {
        mmu.write_byte(0xA5, TILE_DATA_ADDR0_START + j);
        // for (int i = 0; i < 16; i += 4)
        // {
        //     mmu.write_byte(0xAA, TILE_DATA_ADDR0_START + j);
        //     mmu.write_byte(0xAA, TILE_DATA_ADDR0_START + j + 1 + i);
        //     mmu.write_byte(0x55, TILE_DATA_ADDR0_START + j + 2 + i);
        //     mmu.write_byte(0x55, TILE_DATA_ADDR0_START + j + 3 + i);
        // }
    }

    auto start = std::chrono::steady_clock::now();

    ppu.render_scanline();

    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;

    std::cout << diff.count();

    display.update_screen();


    SDL_Delay(3000);
}

void speed_test()
{
    Mmu mmu;
    Ppu ppu(&mmu);

    uint32_t buffer[GBResolution::HEIGHT * GBResolution::WIDTH]{};
    std::fill(buffer, buffer + sizeof(buffer) / sizeof(buffer[0]), GBColours::COLOUR10);

    Display display(&ppu);

    mmu.write_byte(0x27, 0); // DAA
    mmu.write_byte(0x00, 1); // NOP
    mmu.write_byte(0xC7, 2); // RST 0x00

    Cpu cpu(mmu);

    auto start = std::chrono::steady_clock::now();
    // for (uint32_t i = 0; i < CYCLES_PER_FRAME;)
    // {
    //     i += cpu.execute_instruction();
    // }

    display.update_screen(buffer); // SDL bottleneck - takes between 3ms to 9ms

    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;

    std::cout << diff.count();
}