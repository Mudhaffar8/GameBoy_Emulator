#pragma once

#include <cstdint>
#include <vector>
#include <iostream>

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
    constexpr int HEIGHT_SPRITE_PIXELS = 16;

    constexpr int BYTES_PER_TILE = 16;
    constexpr int BYTES_PER_ROW = 2;
}

/// @note Implementation assumes your system is little-endian. 
struct GBSprite
{
    uint8_t y_pos{}; // 16 would place it at top
    uint8_t x_pos{}; // 8 would place it to left
    uint8_t tile_number{}; // always uses 8000 method
    union
    {
        uint8_t attributes{};
        struct 
        {
            uint8_t cgb_pallete : 3; // CGB Only
            uint8_t bank : 1; // CGB Only
            uint8_t dmg_pallete_number : 1; // If set to 0, the OBP0 register is used as the palette, otherwise OBP1
            uint8_t x_flip : 1;
            uint8_t y_flip : 1;
            uint8_t obj_to_bg_priority : 1; // 0 = sprite always above bg, 1 = bg colors 1-3 overlay sprite, sprite still rendered above 0
        };  
    };

    GBSprite() {}
    GBSprite(uint8_t y, uint8_t x, uint8_t tile_num, uint8_t attrib) : 
        y_pos(y), 
        x_pos(x), 
        tile_number(tile_num), 
        attributes(attrib) 
    {}

    bool operator<(const GBSprite& rhs) const
    {
        return x_pos < rhs.x_pos;
    }

    friend std::ostream& operator<<(std::ostream& os, const GBSprite& sprite) 
    {
        os << "Sprite: (X: " << +sprite.x_pos << " Y: " << +sprite.y_pos
            << "), Tile num: " << +sprite.tile_number 
            << ", attributes: " << +sprite.attributes;
        return os; 
    }
};
static_assert(sizeof(GBSprite) == 4);

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

    enum class TileMaps 
    {
        Background,
        Window
    };

    enum class LCDC
    {
        BgWindowEnable = 0x01, // Enables/Disables BG & Window; both become blank (white)
        ObjEnable = 0x02, // Enables objects (0 = Off; 1 = On)
        ObjSize =  0x04, // 0 = 8x8, 1 = 8x16
        BgTileMapArea = 0x08, // 0 - 9800-9BFF; 1 = 9C00-9FFF
        BgWindowTileDataArea = 0x10, // 0 = 8800-97FF; 1 = 8000-8FFF
        WindowEnable = 0x20,
        WindowTileMap = 0x40, // 0 = 9800-9BFF; 1 = 9C00-9FFF
        LCDPpuEnable = 0x80 // 0 = Off; 1 - On
    };

    enum class LCDStatus
    {
        PpuMode = 0b11,
        LycEqualsLy = 0b100,
        Mode0Select = 0x08,
        Mode1Select = 0x10,
        Mode2Select = 0x20,
        LycSelect = 0x40,
    };

    bool debug_mode = false;

    /// @brief Advance PPU by a given number of cycles
    /// @param cycles Number of CPU cycles to advance.
    void tick(uint32_t cycles);

    /* OAM Scan */
    void oam_scan(int screen_y);

    /* Rendering Methods */
    // Scanline Rendering
    void render_scanline(int screen_y);
    void render_bg_scanline(int screen_y);
    void render_window_scanline(int screen_y);
    void render_sprites_scanline(int screen_y);

    // Tile Rendering
    void render_tile(int tile_x, int tile_y);

    // Frame Rendering
    void render_frame();
    void render_bg_frame();
    void render_window_frame();
    void render_sprites_frame();

    /* Fill Screen with Colour */
    void reset_screen();
    void fill_white_screen();

    /* Tile Methods */
    void decode_tile_row(uint8_t hi_byte, uint8_t lo_byte, int tile_screen_x, int screen_y);
    uint32_t get_tile_colour(uint8_t bit2) const;
    std::pair<uint8_t, uint8_t> fetch_tile_row(int tile_map_x, int tile_map_y, TileMaps layer) const;
    std::pair<uint8_t, uint8_t> fetch_tile_row(int tile_id, int tile_map_y) const; // For Sprites

    /// @brief Frame buffer containing 32-bit RGBA pixel values.
    /// @todo Make this private.
    std::array<uint32_t, GBResolution::DIMENSIONS> frame_buffer{};

    // LCDC Methods
    inline void set_lcdc(LCDC lcdc_bit) { lcdc |= static_cast<uint8_t>(lcdc_bit); }
    inline bool check_lcdc(LCDC lcdc_bit) { return (lcdc & static_cast<uint8_t>(lcdc_bit)) != 0; }
    
    // @brief Temporary setters for scroll values
    void set_bg_scroll_values(uint8_t scx, uint8_t scy)
    {
        this->bg_scroll_x = scx;
        this->bg_scroll_y = scy;
    }

    void set_window_scroll_values(uint8_t scx, uint8_t scy)
    {
        this->window_scroll_x = scx;
        this->window_scroll_y = scy;
    }
private:
    Mmu& mmu;

    uint8_t& lcdc; // LCD Control
    uint8_t& lcd_status; // LCD Status

    std::vector<GBSprite> oam_buffer;
    
    Mode ppu_mode = Mode::OamScan;

    uint8_t bg_scroll_x = 0;
    uint8_t bg_scroll_y = 0;

    uint8_t window_scroll_x = 7;
    uint8_t window_scroll_y = 0;

    int scanline_y = 0;

    uint32_t cycles_elapsed = 0;

    void update_ppu_mode(Mode new_mode);
};