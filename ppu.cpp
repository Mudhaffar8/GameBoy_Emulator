#include "ppu.hpp"

Ppu::Ppu(Memory& _mem) :
    mem(_mem),
    ppu_mode(PpuModes::OamScan),
    cycles_elapsed(0)
{}

void Ppu::tick(uint32_t cycles)
{}