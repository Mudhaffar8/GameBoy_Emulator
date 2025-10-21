#pragma once

#include <cstdint>

#include "memory.hpp"

constexpr int GAMEBOY_SCREEN_WIDTH = 160;
constexpr int GAMEBOY_SCREEN_HEIGHT = 144;

// Order probably be wrong
struct GbObject
{
    struct 
    {
        uint8_t cgb_flags : 4; // Unused for now
        uint8_t pallet_number : 1;
        uint8_t x_flip : 1;
        uint8_t y_flip : 1;
        uint8_t priority : 1;
    };  

    uint8_t tile_number;
    uint8_t y_pos;
    uint8_t x_pos;
};

class Ppu
{
public:
    Ppu(Memory& mem);

    void tick(uint32_t cycles);

private:
    Memory& mem;

    enum class PpuModes
    {
        HBlank,
        VBlank,
        OamScan,
        Drawing
    };

    PpuModes ppu_mode;

    uint32_t cycles_elapsed;
};