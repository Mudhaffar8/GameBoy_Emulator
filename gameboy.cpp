#include "gameboy.hpp"

Gameboy::Gameboy(const std::string& rom_name) : 
    mmu(),
    cpu(mmu),
    ppu(mmu),
    joypad(mmu),
    timer(mmu),
    display(ppu)
{}

void Gameboy::tick()
{
    uint32_t cycles = cpu.execute_instruction();
    ppu.tick(cycles);
    timer.tick(cycles);

    cycles_elapsed += cycles;
}