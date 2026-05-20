#pragma once

#include <cstdint>

#include "memory.hpp"
#include "interrupts.hpp"

namespace GBResolution
{
    constexpr int WIDTH = 160;
    constexpr int HEIGHT = 144;
    constexpr int DIMENSIONS = WIDTH * HEIGHT;

    constexpr int TILE_MAP_SIZE_PIXELS = 256;

    constexpr int TILES_PER_ROW_VISIBLE_MIN = 20;
    constexpr int TILES_PER_ROW_VISIBLE_MAX = 21;
    constexpr int TILES_PER_ROW = 32;
}

namespace GBColours
{
    constexpr uint32_t COLOUR_00 = 0xFFFFFFFF; // White
    constexpr uint32_t COLOUR_01 = 0X7F7F7FFF; // Light Gray
    constexpr uint32_t COLOUR_10 = 0x3F3F3FFF; // Dark Gray
    constexpr uint32_t COLOUR_11 = 0x000000FF; // Black
}

namespace GBTiming
{
    constexpr int VBLANK_LINE_COUNT = 10;
    constexpr int TOTAL_SCANLINES = GBResolution::HEIGHT + VBLANK_LINE_COUNT;

    constexpr int CYCLES_PER_SCANLINE = 456;
    constexpr int CYCLES_PER_FRAME = CYCLES_PER_SCANLINE * TOTAL_SCANLINES;
    constexpr int CYCLES_OAM_SCAN = 80;
}

namespace GBTile
{
    constexpr int SIZE_PIXELS = 8;

    constexpr int BYTES_PER_TILE = 16;
    constexpr int BYTES_PER_ROW = 2;
}

/// @note Implementation assumes your system is little-endian. 
struct GBObject
{
    uint8_t y_pos; // 16 would place it at top
    uint8_t x_pos; // 8 would place it to left
    uint8_t tile_number; // always uses 8000 method
    union
    {
        uint8_t attributes;
        struct 
        {
            uint8_t cgb_flags : 4; // Unused for now
            uint8_t pallete_number : 1; // If set to 0, the OBP0 register is used as the palette, otherwise OBP1
            uint8_t x_flip : 1;
            uint8_t y_flip : 1;
            uint8_t obj_to_bg_priority : 1; // 0 = sprite always above bg, 1 = bg colors 1-3 overlay sprite, sprite still rendered above 0
        };  
    };

    GBObject() : y_pos(), x_pos(), tile_number(), attributes() {}
};
static_assert(sizeof(GBObject) == 4);

/// @brief Emulates Game Boy PPU (Pixel Processing Unit.)
class Ppu
{
public:
    explicit Ppu(Mmu& mmu);

    enum class Mode : uint8_t
    {
        HBlank,
        VBlank,
        OamScan,
        Drawing
    };

    void test();

    /// @brief Advance PPU by a given number of cycles
    /// @param cycles Number of CPU cycles to advance.
    void tick(uint32_t cycles);

    // Rendering Methods
    void render_bg_scanline(int y);
    void render_tile(int tile_x, int tile_y);
    void render_frame();

    void reset_screen();

    // Tile Methods
    void decode_tile_row(uint8_t hi_byte, uint8_t lo_byte, int x, int j);
    uint32_t get_tile_colour(uint8_t bit2) const;
    std::pair<uint8_t, uint8_t> fetch_tile_row(int bg_map_x, int bg_map_y) const;

    /// @brief Frame buffer containing 32-bit RGBA pixel values.
    /// @todo Make this private.
    std::array<uint32_t, GBResolution::DIMENSIONS> frame_buffer{};

    // @brief Temporary setters for scroll values
    void set_scroll_values(uint8_t scx, uint8_t scy)
    {
        this->scroll_x = scx;
        this->scroll_y = scy;
    }

    bool debug_mode = false;
private:
    Mmu& mmu;

    Mode ppu_mode = Mode::OamScan;

    uint8_t scroll_x = 0;
    uint8_t scroll_y = 0;

    int scanline_x = 0;
    int scanline_y = 0;

    uint32_t cycles_elapsed = 0;
};