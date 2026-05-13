#include <SDL3/SDL.h>
#include <SDL3/SDL_keyboard.h>

#include <assert.h>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>

#include "cpu.hpp"
#include "display.hpp"
#include "memory.hpp"
#include "ppu.hpp"

double speed_test();
void ppu_test();
void ppu_tile_test(int tile_x, int tile_y);
void avg_speed_test();


int main(int argc, char** argv)
{
    assert(argc > 2);

    ppu_tile_test(
        std::atoi(argv[1]),
        std::atoi(argv[2])
    );

    return 0;
}

void ppu_tile_test(int tile_x, int tile_y)
{
    Mmu mmu;
    Ppu ppu(mmu);
    Display display(ppu);

    ppu.render_tile(tile_x, tile_y);
    display.update_screen();

    SDL_Delay(3000);
}

void ppu_test()
{
    Mmu mmu;
    Ppu ppu(mmu);
    Display display(ppu);

    auto start = std::chrono::steady_clock::now();

    ppu.render_frame();
    display.update_screen();

    auto end = std::chrono::steady_clock::now();
    double diff = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << diff;
    
    SDL_Delay(3000);
}

void avg_speed_test()
{
    constexpr int NUM_OF_TESTS = 1000;

    std::vector<double> times;
    times.reserve(NUM_OF_TESTS);
    
    for (int i = 0; i < NUM_OF_TESTS; ++i) 
        times.push_back(speed_test());   

    double avg = std::accumulate(times.begin(), times.end(), 0.0);
    avg = avg / static_cast<double>(NUM_OF_TESTS);

    std::cout << avg;
}

double speed_test()
{
    Mmu mmu;
    Ppu ppu(mmu);

    uint32_t buffer[GBResolution::HEIGHT * GBResolution::WIDTH]{};
    std::fill(buffer, buffer + sizeof(buffer) / sizeof(buffer[0]), GBColours::COLOUR_10);

    Display display(ppu);

    mmu.write_byte(0x27, 0); // DAA
    mmu.write_byte(0x0F, 1); // RLCA
    mmu.write_byte(0x99, 3); // SBC A, C
    mmu.write_byte(0xC7, 5); // RST 0x00

    Cpu cpu(mmu);

    auto start = std::chrono::steady_clock::now();
    for (uint32_t i = 0; i < GBTiming::CYCLES_PER_FRAME;)
    {
        i += cpu.execute_instruction();
    }

    display.update_screen(buffer); 

    auto end = std::chrono::steady_clock::now();
    double diff = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << diff;

    SDL_Delay(1000);
    
    return diff;
}