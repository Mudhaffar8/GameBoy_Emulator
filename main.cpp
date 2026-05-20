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
void ppu_scroll_test2();
void ppu_scroll_test(int scx, int scy);
void ppu_scanline_test(int scx);
void ppu_tile_test();
void avg_speed_test();

static Mmu mmu;
static Cpu cpu(mmu);
static Ppu ppu(mmu);
static Display display(ppu);

int main(int argc, char** argv)
{
    int scx = std::atoi(argv[2]);
    int scy = std::atoi(argv[3]);
    bool debug_mode_enabled = std::string(argv[4]) == "t";

    ppu.debug_mode = debug_mode_enabled;

    if (argv[1] == std::string("scroll_test"))
        ppu_scroll_test(scx, scy);
    else if (argv[1] == std::string("scroll_test2"))
        ppu_scroll_test2();
    else if (argv[1] == std::string("scanline_test"))
        ppu_scanline_test(scx);

    return 0;
}

void ppu_scanline_test(int scx)
{
    ppu.set_scroll_values(scx, 0);

    ppu.render_bg_scanline(0);
    display.update_screen();

    SDL_Delay(3000);
}

void ppu_scroll_test(int scx, int scy)
{
    ppu.set_scroll_values(scx, scy);

    ppu.render_frame();
    display.update_screen();

    SDL_Delay(3000);
}

void ppu_tile_test()
{
    ppu.render_tile(0, 0);
    ppu.render_tile(19, 0);
    display.update_screen();

    SDL_Delay(3000);
}

void ppu_scroll_test2()
{
    auto start = std::chrono::steady_clock::now();
    
    ppu.render_frame();
    display.update_screen();

    SDL_Delay(1000);
    
    for (uint32_t i = 0; i < (GBResolution::TILE_MAP_SIZE_PIXELS * 2); ++i)
    {
        ppu.set_scroll_values(i, i);

        ppu.render_frame();
        display.update_screen();

        // std::cout << "Scroll = (X: " << i << ", Y: " << i << ")\n";

        SDL_Delay(10);
    }

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
    avg /= static_cast<double>(NUM_OF_TESTS);

    std::cout << avg;
}

double speed_test()
{
    uint32_t buffer[GBResolution::HEIGHT * GBResolution::WIDTH]{};
    std::fill(buffer, buffer + sizeof(buffer) / sizeof(buffer[0]), GBColours::COLOUR_10);

    Display display(ppu);

    mmu.write_byte(0x27, 0); // DAA
    mmu.write_byte(0x0F, 1); // RLCA
    mmu.write_byte(0x99, 3); // SBC A, C
    mmu.write_byte(0xC7, 5); // RST 0x00

    auto start = std::chrono::steady_clock::now();
    for (uint32_t i = 0; i < GBTiming::CYCLES_PER_FRAME; i += cpu.execute_instruction());

    display.update_screen(buffer); 

    auto end = std::chrono::steady_clock::now();
    double diff = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << diff;

    SDL_Delay(1000);
    
    return diff;
}