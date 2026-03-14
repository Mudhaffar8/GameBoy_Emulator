#pragma once

#include <queue>
#include <cstdint>

#include "memory.hpp"
#include "interrupts.hpp"

namespace GBResolution
{
    constexpr int WIDTH = 160;
    constexpr int WIDTH_WHOLE = 256;
    constexpr int HEIGHT = 144;
    constexpr int DIMENSIONS = WIDTH * HEIGHT;
    constexpr int NUM_OF_TILES = 20;
    constexpr int NUM_OF_TILES_WHOLE = 32;
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

constexpr int NUM_TILES_X = 20;
constexpr int NUM_TILES_TOTAL = NUM_TILES_X * NUM_TILES_X;

const int TILE_DATA_LEN = 16;
const int TILE_DATA_LINE_LEN = 2;

constexpr int NUM_OF_SCANLINES = 144;
constexpr int NUM_OF_VISIBLE_SCANLINES = NUM_OF_SCANLINES - 10;

/// @brief Emulates Game Boy PPU (Pixel Processing Unit).
class Ppu
{
public:
    Ppu(Mmu& mmu);

    void test();

    /// @brief Advance PPU by a given number of cycles
    /// @param cycles Number of CPU cycles to advance.
    void tick(uint32_t cycles);

    // Rendering Methods
    void render_scanline();
    void render_frame();
    void render_tile();

    uint32_t get_tile_colour(uint8_t bit2);
    void decode_tile_row(uint8_t hi_byte, uint8_t lo_byte, int x, int j);
    std::pair<uint8_t, uint8_t> fetch_tile_row(int bg_map_x, int bg_map_y);

    /// @brief Frame buffer containing 32-bit RGBA pixel values.
    /// @todo Make this private.
    std::array<uint32_t, GBResolution::DIMENSIONS> frame_buffer{};
private:
    Mmu& mmu;

    enum class PpuModes : uint8_t
    {
        HBlank,
        VBlank,
        OamScan,
        Drawing
    };

    PpuModes ppu_mode = PpuModes::HBlank;

    int scanline_x = 0;
    int scanline_y = 0;

    int scroll_x = 0;
    int scroll_y = 0;

    uint32_t cycles_elapsed = 0;
};