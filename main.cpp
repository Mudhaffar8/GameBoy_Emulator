#include <SDL3/SDL.h>
#include <SDL3/SDL_keyboard.h>

#include <iostream>
#include <chrono>
#include <cstdint>
#include <numeric>

#include "cpu.hpp"
#include "ppu.hpp"
#include "memory.hpp"
#include "display.hpp"
#include "cpu.hpp"

double speed_test();
void ppu_test();

int main(int argc, char** argv)
{
    constexpr int NUM_OF_TESTS = 1000;

    std::vector<double> times;
    times.reserve(NUM_OF_TESTS);
    for (int i = 0; i < NUM_OF_TESTS; ++i) times.push_back(speed_test());   

    double avg = std::accumulate(times.begin(), times.end(), 0.0);
    avg = avg / static_cast<double>(NUM_OF_TESTS);

    std::cout << avg;

    return 0;
}

void ppu_test()
{
    Mmu mmu;
    Ppu ppu(mmu);
    Display display(ppu);

    auto start = std::chrono::high_resolution_clock::now();

    ppu.render_frame();
    display.update_screen();

    auto end = std::chrono::high_resolution_clock::now();
    double diff = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << diff;

    SDL_Delay(3000);
}

double speed_test()
{
    Mmu mmu;
    Ppu ppu(mmu);

    uint32_t buffer[GBResolution::HEIGHT * GBResolution::WIDTH]{};
    std::fill(buffer, buffer + sizeof(buffer) / sizeof(buffer[0]), GBColours::COLOUR10);

    //Display display(ppu);

    mmu.write_byte(0x27, 0); // DAA
    mmu.write_byte(0x0F, 1); // RLCA
    mmu.write_byte(0xCB, 2); // RLC [HL]
    mmu.write_byte(0x06, 3);
    mmu.write_byte(0xC7, 4); // RST 0x00

    Cpu cpu(mmu);

    auto start = std::chrono::high_resolution_clock::now();
    for (uint32_t i = 0; i < CYCLES_PER_FRAME;)
    {
        i += cpu.execute_instruction();
    }

    //display.update_screen(buffer); 

    auto end = std::chrono::high_resolution_clock::now();
    double diff = std::chrono::duration<double, std::milli>(end - start).count();

    //std::cout << diff;
    
    return diff;
}