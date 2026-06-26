#pragma once

#include <cstdint>
#include <vector>
#include <iostream>
#include <cstddef>
#include <bitset>

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
    constexpr int CYCLES_DRAWING_MIN = 172; // Can range from 172 to 289
    constexpr int CYCLES_HBLANK = CYCLES_PER_SCANLINE - CYCLES_DRAWING_MIN - CYCLES_OAM_SCAN;
}

namespace GBTile
{
    constexpr int SIZE_PIXELS = 8;
    constexpr int MAX_HEIGHT_SPRITE_PIXELS = 16;

    constexpr int BYTES_PER_TILE = 16;
    constexpr int BYTES_PER_ROW = 2;
}

namespace GBPalette
{
    constexpr int BYTES_PER_PALETTE = 8;
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
            uint8_t cgb_palette : 3; // Which of OBP0-7 to use
            uint8_t vram_bank : 1; // 0 = Fetch tile from VRAM bank 0, 1 = Fetch tile from VRAM bank 1
            uint8_t dmg_palette_number : 1; // If set to 0, the OBP0 register is used as the palette, otherwise OBP1
            uint8_t flip_x : 1;
            uint8_t flip_y : 1;
            // OAM Attributes bit 7 will grant OBJ priority when clear, not when set.
            // Priority between all OBJs is resolved before priority with the BG layer is considered.
            // 0 = sprite always above bg, 1 = bg colors 1-3 overlay sprite, sprite still rendered above 0
            uint8_t bg_priority : 1;
        };  
    };

    GBSprite() {}
    GBSprite(uint8_t y, uint8_t x, uint8_t tile_num, uint8_t attrib) : 
        y_pos(y), 
        x_pos(x), 
        tile_number(tile_num), 
        attributes(attrib) 
    {}

    friend std::ostream& operator<<(std::ostream& os, const GBSprite& sprite) 
    {
        std::byte b{sprite.attributes};
        os << "Sprite: (X: " << +sprite.x_pos << " Y: " << +sprite.y_pos
            << "), Tile num: " << +sprite.tile_number 
            << ", attributes: " << std::bitset<8>(std::to_integer<int>(b));
        return os; 
    }
};
static_assert(sizeof(GBSprite) == sizeof(uint32_t));

/// @brief Emulates Game Boy PPU (Pixel Processing Unit.)
class Ppu
{
public:
    explicit Ppu(Mmu& mmu);

    enum class Mode : uint8_t
    {
        HBlank, // VRAM Accessible: VRAM, OAM, CGB palettes
        VBlank, // VRAM Accessible: VRAM, OAM, CGB palettes
        OamScan, // VRAM Accessible: VRAM, CGB palettes
        Drawing // VRAM Accessible: None
    };

    enum class LCDC : uint8_t
    {
        BgWindowEnable = 0x01, // Enables/Disables BG & Window; both become blank (white)
        ObjEnable = 0x02, // Enables objects (0 = Off; 1 = On)
        ObjSize =  0x04, // 0 = 8x8, 1 = 8x16
        BgTileMapArea = 0x08, // 0 - 9800-9BFF; 1 = 9C00-9FFF
        BgWindowTileDataArea = 0x10, // 0 = 8800-97FF (8800 method); 1 = 8000-8FFF (8000 method)
        WindowEnable = 0x20,
        WindowTileMap = 0x40, // 0 = 9800-9BFF; 1 = 9C00-9FFF
        LCDPpuEnable = 0x80 // 0 = Off; 1 - On (Not Implemented)
    };

    enum class LCDStatus : uint8_t
    {
        PpuMode = 0b11, // (Read-only): Indicates the PPU’s current status. Reports 0 instead when the PPU is disabled.
        Coincidence = 0x04, // (Read-only): Set when LY contains the same value as LYC; it is constantly updated.
        Mode0Select = 0x08, // (Read/Write): If set, selects the Mode 0 (HBlank) condition for the STAT interrupt.
        Mode1Select = 0x10, // (Read/Write): If set, selects the Mode 1 (VBlank) condition for the STAT interrupt.
        Mode2Select = 0x20, // (Read/Write): If set, selects the Mode 2 (OAM Scan) condition for the STAT interrupt.
        LycIntSelect = 0x40, // (Read/Write): If set, selects the LYC == LY condition for the STAT interrupt.
        Unused = 0x80 // Unused (Always 1)
    };

    enum class BGMapAttributes
    {
        ColourPalette = 0b111, // Color palette: Which of BGP0–7 to use
        Bank = 0x08, // Bank: 0 = Fetch tile from VRAM bank 0; 1 = Fetch tile from VRAM bank 1
        Unused = 0x10,
        XFlip = 0x20,
        YFilp = 0x40,
        Priority = 0x80 // Priority: 0 = No; 1 = Color indices 1–3 of the corresponding BG/Window tile are drawn over OBJ, regardless of OBJ priority
    };

    /*
        If the BG color index is 0, the OBJ will always have priority;
        Otherwise, if LCDC bit 0 is clear, the OBJ will always have priority;
        Otherwise, if both the BG Attributes and the OAM Attributes have bit 7 clear, the OBJ will have priority;
        Otherwise, BG will have priority.
    */

    /// @brief Frame buffer containing 32-bit RGBA pixel values.
    /// @todo Make this private.
    std::array<uint32_t, GBResolution::DIMENSIONS> frame_buffer{};

    bool debug_mode = false;
    
    /// @brief Advance PPU by a given number of cycles
    /// @param cycles Number of CPU cycles to advance.
    void tick(uint32_t cycles);

    /* OAM Scan */
    void oam_scan(uint8_t screen_y);

    /* Rendering Methods */
    // Scanline Rendering
    void render_scanline(uint8_t screen_y);
    void render_bg_scanline(uint8_t screen_y);
    void render_window_scanline(uint8_t screen_y);
    void render_sprites_scanline(uint8_t screen_y);

    // Frame Rendering
    void render_frame();
    void render_bg_frame();
    void render_window_frame();
    void render_sprites_frame();

    /* Fill Screen with Colour */
    void reset_screen();
    void fill_white_screen();

    /* Tile Methods */
    // Tile Row Fetching
    std::pair<uint8_t, uint8_t> fetch_tile_row(int tile_map_x, int tile_map_y, bool use_bg_tile_map, bool flip_y) const; // For Window & Background Layers
    std::pair<uint8_t, uint8_t> fetch_sprite_tile_row(int tile_id, int tile_map_y, bool fetch_vram_bank_1) const; // For Sprites Only
    
    // Tile Row Decoding
    std::array<uint8_t, 8> decode_tile_row(uint8_t hi_byte, uint8_t lo_byte);
   
    // Writing to Frame Buffer
    void write_pixels(std::array<uint8_t, 8>& tile_pixels, std::array<uint8_t, 8>& palette, int screen_x, int screen_y, bool flip_x);
    void write_sprite_pixels(std::array<uint8_t, 8>& tile_pixels, std::array<uint8_t, 8>& palette, const GBSprite& sprite, int screen_x, int screen_y);
    
    /* Palettes */
    void refresh_palettes();
    std::array<uint8_t, 8> get_cgb_palette(std::array<uint8_t, 64>& palette, uint8_t palette_num);
    uint32_t convert_rgb555_to_rgba32(uint8_t high_byte, uint8_t low_byte);

    /* Fetch BG Map Attributes */
    uint8_t fetch_bg_map_attributes(int tile_map_x, int tile_map_y, bool use_9C00_tile_map);

    inline bool check_bg_map_attrib(BGMapAttributes attrib_bit, uint8_t byte) const 
    { 
        return (byte & static_cast<uint8_t>(attrib_bit)) != 0; 
    }

    /* LCDC Methods */
    inline void set_lcdc(LCDC lcdc_bit, bool cond) 
    { 
        lcdc = (cond) ? 
            (lcdc | static_cast<uint8_t>(lcdc_bit)) : 
            (lcdc & ~static_cast<uint8_t>(lcdc_bit));
    }
    inline bool check_lcdc(LCDC lcdc_bit) const { return (lcdc & static_cast<uint8_t>(lcdc_bit)) != 0; }

    /* LCD Status Methods */
    inline void set_lcd_status(LCDStatus lcd_status_bit, bool cond) 
    { 
        lcd_status = (cond) ? 
            (lcd_status | static_cast<uint8_t>(lcd_status_bit)) : 
            (lcd_status & ~static_cast<uint8_t>(lcd_status_bit));
    }
    inline bool check_lcd_status(LCDStatus lcd_status_bit) const { return (lcd_status & static_cast<uint8_t>(lcd_status_bit)) != 0; }
    
    /* Temporary setters for BG & Window scroll values */
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

    // PPU Register References
    uint8_t& lcdc; // LCD Control
    uint8_t& lcd_status; // LCD Status
    uint8_t& ly_compare; // LCD Y Coordinate
    uint8_t& bg_palette;
    uint8_t& obj0_palette;
    uint8_t& obj1_palette;
    uint8_t& bg_scroll_x;
    uint8_t& bg_scroll_y;
    uint8_t& window_scroll_x;
    uint8_t& window_scroll_y;
    uint8_t& scanline_y;

    std::vector<GBSprite> oam_buffer{};

    // Keep track of raw colour indices per scanline
    std::array<uint8_t, GBResolution::WIDTH> scanline_buffer{}; 
    
    Mode ppu_mode = Mode::OamScan;

    uint32_t cycles_elapsed = 0;

    uint8_t window_internal_scanline_y{};

    /* CGB Palettes */
    std::array<std::array<uint8_t, 8>, 8> obj_palettes;
    std::array<std::array<uint8_t, 8>, 8> bg_palettes;

    /* Mode Switching */
    void update_ppu_mode(Mode new_mode);

    inline void set_scanline(uint8_t new_scanline);
    inline void update_coincidence_flag();
};