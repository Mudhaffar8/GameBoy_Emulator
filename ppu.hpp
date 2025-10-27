#pragma once

#include <queue>
#include <cstdint>

#include "memory.hpp"
#include "interrupts.hpp"

namespace GBResolution
{
constexpr int WIDTH = 160;
constexpr int HEIGHT = 144;
constexpr int DIMENSIONS = WIDTH * HEIGHT;
constexpr int NUM_OF_TILES = 32;
}

namespace GBColours
{
static const uint32_t COLOUR00 = 0xFFFFFFFF;
static const uint32_t COLOUR01 = 0X7F7F7FFF;
static const uint32_t COLOUR10 = 0x3F3F3FFF;
static const uint32_t COLOUR11 = 0x000000FF;
}


const int CYCLES_PER_SCANLINE = 456;
const int CYCLES_PER_FRAME = 70224;
const int CYCLES_OAM = 80;

// Order probably be wrong
struct GBObject
{
    struct 
    {
        uint8_t cgb_flags : 4; // Unused for now
        uint8_t pallet_number : 1;
        uint8_t x_flip : 1;
        uint8_t y_flip : 1;
        uint8_t obj_to_bg_priority : 1;
    };  

    uint8_t tile_number;
    uint8_t y_pos;
    uint8_t x_pos;
};

struct GBPixelFIFO
{
    uint8_t color;
    uint8_t pallete;
    uint8_t priority;
};

class Ppu
{
public:
    Ppu(Mmu* mmu);

    void test();

    void tick(uint32_t cycles);

    void render_frame();
    void render_tile();
    void render_scanline();
    void decode_tile_row(uint8_t hi_byte, uint8_t lo_byte, int x, int j);
    std::pair<uint8_t, uint8_t> fetch_tile_row(int tile_row_i);
    uint32_t get_tile_colour(uint8_t bit2);

    uint32_t frame_buffer[GBResolution::DIMENSIONS]{};
private:
    Mmu* mmu;

    enum class PpuModes
    {
        HBlank,
        VBlank,
        OamScan,
        Drawing
    };

    std::queue<GBPixelFIFO> pixel_fifo;

    PpuModes ppu_mode = PpuModes::HBlank;

    int tile_map_i = 0; // Index of the tile map currently being operated on
    uint32_t cycles_elapsed = 0;
};