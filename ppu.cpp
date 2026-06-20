#include <stdexcept>
#include <iostream>
#include <cassert>
#include <cstring>
#include <algorithm>

#include "ppu.hpp"

Ppu::Ppu(Mmu& _mmu) :
    mmu(_mmu),
    lcdc(mmu.read_io_reg(LCD_CONTROL)),
    lcd_status(mmu.read_io_reg(LCD_STATUS)),
    ly_compare(mmu.read_io_reg(LY_COMPARE)),
    bg_palette(mmu.read_io_reg(BG_PALETTE)),
    obj0_palette(mmu.read_io_reg(OBJ_PALETTE_0)),
    obj1_palette(mmu.read_io_reg(OBJ_PALETTE_1)),
    bg_scroll_x(mmu.read_io_reg(VIEWPORT_X_POS)),
    bg_scroll_y(mmu.read_io_reg(VIEWPORT_Y_POS)),
    window_scroll_x(mmu.read_io_reg(WINDOW_X_POS)),
    window_scroll_y(mmu.read_io_reg(WINDOW_Y_POS)),
    scanline_y(mmu.read_io_reg(LCD_Y_COORDINATE))
{
    oam_buffer.reserve(10);
}

// While the PPU is accessing some video-related memory, 
// that memory is inaccessible to the CPU (writes are ignored, and reads return garbage values, usually $FF).
void Ppu::tick(uint32_t cycles)
{
    cycles_elapsed += cycles;

    switch (ppu_mode)
    {
    case Mode::OamScan:
        if (cycles_elapsed >= GBTiming::CYCLES_OAM_SCAN)
        {
            oam_scan(scanline_y);
            update_ppu_mode(Mode::Drawing);
        }
        break;
    case Mode::Drawing:
        if (cycles_elapsed >= GBTiming::CYCLES_OAM_SCAN + GBTiming::CYCLES_DRAWING_MIN)
        {
            render_scanline(scanline_y);
            update_ppu_mode(Mode::HBlank);
        }
        break;
    case Mode::HBlank:
        if (cycles_elapsed >= GBTiming::CYCLES_PER_SCANLINE)
        {
            cycles_elapsed %= GBTiming::CYCLES_PER_SCANLINE;
            set_scanline(scanline_y + 1);

            if (scanline_y >= GBResolution::HEIGHT)
                update_ppu_mode(Mode::VBlank);
            else
                update_ppu_mode(Mode::OamScan);
        }
        break;
    case Mode::VBlank:
        if (cycles_elapsed >= GBTiming::CYCLES_PER_SCANLINE)
        {
            cycles_elapsed %= GBTiming::CYCLES_PER_SCANLINE;
            set_scanline(scanline_y + 1);

            if (scanline_y >= GBTiming::TOTAL_SCANLINES)
            {
                set_scanline(0);
                update_ppu_mode(Mode::OamScan);
            }
        }
        break;
    }
}

/* Scanline Methods */
inline void Ppu::set_scanline(uint8_t new_scanline)
{
    scanline_y = new_scanline;
    update_coincidence_flag();

    if (check_lcd_status(LCDStatus::LycIntSelect))
    {
        if (scanline_y == ly_compare)
            GBInterrupts::request_interrupt(mmu, Interrupts::LCD);
    }
}

inline void Ppu::update_coincidence_flag()
{
    if (scanline_y == ly_compare)
        set_lcd_status(LCDStatus::Coincidence, true);
    else
        set_lcd_status(LCDStatus::Coincidence, false);
}

/* PPU Mode Switching */
void Ppu::update_ppu_mode(Mode new_mode)
{
    ppu_mode = new_mode;
    lcd_status = (lcd_status & 0xFC) | static_cast<uint8_t>(ppu_mode);

    switch(new_mode)
    {
    case Mode::HBlank:
        if (check_lcd_status(LCDStatus::Mode0Select))
            GBInterrupts::request_interrupt(mmu, Interrupts::LCD);

        break;
        
    case Mode::VBlank:
        window_internal_scanline_y = 0;
        
        GBInterrupts::request_interrupt(mmu, Interrupts::VBlank);

        if (check_lcd_status(LCDStatus::Mode1Select))
            GBInterrupts::request_interrupt(mmu, Interrupts::LCD);

        break;

    case Mode::OamScan:
        if (check_lcd_status(LCDStatus::Mode2Select))
            GBInterrupts::request_interrupt(mmu, Interrupts::LCD);

        break;
    
    case Mode::Drawing:
        break;
    }
}
 
/// @brief Converts the 2BPP colour value into 32-bit RGBA.
/// @param bit2 
/// @returns a 32-bit RGBA colour value.
/// @throws index out of bounds exception if bit2 > 3.
uint32_t Ppu::get_tile_colour(uint8_t bit2) const
{
    static constexpr std::array<uint32_t, 4> palette 
    {
        GBColours::COLOUR_00,
        GBColours::COLOUR_01,
        GBColours::COLOUR_10,
        GBColours::COLOUR_11
    };
    return palette.at(bit2);    
}

std::array<uint8_t, 4> Ppu::get_palette(uint8_t palette8)
{
    std::array<uint8_t, 4> palette{};
    for (int i = 0; i < 4; ++i)
        palette.at(i) = (palette8 >> (2 * i)) & 0b11;

    return palette;
}


void Ppu::render_frame()
{
    window_internal_scanline_y = 0;

    for (int y = 0; y < GBResolution::HEIGHT; ++y) 
    {
        oam_scan(y);
        render_scanline(y);
    }
}

void Ppu::render_scanline(uint8_t screen_y)
{
    if (!check_lcdc(LCDC::LCDPpuEnable)) 
        return;

    std::memset(scanline_buffer.data(), 0x00, scanline_buffer.size());

    bool bg_window_enable = check_lcdc(LCDC::BgWindowEnable);
    if (bg_window_enable)
    {
        render_bg_scanline(screen_y);

        bool window_enable = check_lcdc(LCDC::WindowEnable);
        if (window_enable)
            render_window_scanline(screen_y);
    }
    else
    {
        std::fill(
            frame_buffer.begin() + (GBResolution::WIDTH * screen_y), 
            frame_buffer.begin() + (GBResolution::WIDTH * screen_y) + GBResolution::WIDTH,
            GBColours::COLOUR_00
        );
    }

    bool obj_enable = check_lcdc(LCDC::ObjEnable);
    if (obj_enable)
        render_sprites_scanline(screen_y);
}

/// @brief 
/// @param y the y-position of the scanline in SCREEN coordinates
void Ppu::render_bg_scanline(uint8_t screen_y)
{
    int tile_map_y = (static_cast<int>(screen_y) + bg_scroll_y) & 0xFF;

    // SCX mod 8 => pixels to discard from left
    // 8 - (SCX mod 8) => pixels to discard from right tile 
    int tile_offset_x = bg_scroll_x % GBTile::SIZE_PIXELS;
    
    bool use_9C00_tile_map = check_lcdc(LCDC::BgTileMapArea);

    auto palette = get_palette(bg_palette);

    // 21 unique tiles will need to be rendered at most
    for (int tile_x = 0; tile_x < GBResolution::TILES_PER_ROW_VISIBLE_MAX; ++tile_x)
    {
        int screen_x = tile_x * GBTile::SIZE_PIXELS;
        int tile_map_x = (screen_x + bg_scroll_x) & 0xFF;
        auto tile_row = fetch_tile_row(tile_map_x, tile_map_y, use_9C00_tile_map);

        // Get x-value of leftmost pixel on the tile in SCREEN coordinates
        int last_tile_screen_x = screen_x - tile_offset_x;
        auto tile_pixels = decode_tile_row(tile_row.first, tile_row.second); 

        write_pixels(tile_pixels, last_tile_screen_x, screen_y, palette);
    }
}

void Ppu::render_window_scanline(uint8_t screen_y)
{
    if (screen_y < window_scroll_y || 
        screen_y >= GBResolution::HEIGHT ||
        window_scroll_x >= GBResolution::WIDTH
    ) return;

    int total_scroll_x = window_scroll_x - 7;
    int visible_tiles = (GBResolution::WIDTH - total_scroll_x) / GBTile::SIZE_PIXELS + 1; 

    bool use_9C00_tile_map = check_lcdc(LCDC::WindowTileMap);

    auto palette = get_palette(bg_palette);

    // Given the fact the window does not loop, 
    // there could range from 0 to 21 tiles to render on the screen at any given scanline.
    // Need to calculate based on the window_scroll_x and window_scroll_y values.
    for (int tile_x = 0; tile_x < visible_tiles; ++tile_x)
    {
        int screen_x = tile_x * GBTile::SIZE_PIXELS;
        int tile_map_x = screen_x + total_scroll_x; // Window Layer does not loop
        
        auto tile_row = fetch_tile_row(screen_x, window_internal_scanline_y, use_9C00_tile_map);
        auto tile_pixels = decode_tile_row(tile_row.first, tile_row.second); 
        write_pixels(tile_pixels, tile_map_x, screen_y, palette);
    }

    ++window_internal_scanline_y;
}

void Ppu::render_sprites_scanline(uint8_t screen_y)
{
    bool is_8x16 = check_lcdc(LCDC::ObjSize);
    int max_obj_height_idx = is_8x16 ? 15 : 7;

    auto obj_palette_0 = get_palette(obj0_palette);
    auto obj_palette_1 = get_palette(obj1_palette);

    for (const GBSprite& sprite : oam_buffer)
    {
        // Convert sprite position to screen coordinates
        int obj_screen_y = static_cast<int>(sprite.y_pos) - 16;
        int obj_screen_x = static_cast<int>(sprite.x_pos) - 8;

        int tile_row_y = (sprite.flip_y) ? 
            max_obj_height_idx - (screen_y - obj_screen_y) : 
            screen_y - obj_screen_y; 

        auto& palette = (sprite.dmg_palette_number == 0) ? 
            obj_palette_0 : 
            obj_palette_1;

        int tile_num = (is_8x16) ? (sprite.tile_number & 0xFE) : sprite.tile_number;

        auto tile_row = fetch_sprite_tile_row(tile_num, tile_row_y);
        auto tile_pixels = decode_tile_row(tile_row.first, tile_row.second);
        write_sprite_pixels(tile_pixels, obj_screen_x, screen_y, sprite, palette);
    }
}

// Sprite X-Position must be greater than 0
// LY + 16 must be greater than or equal to Sprite Y-Position
// LY + 16 must be less than Sprite Y-Position + Sprite Height (8 in Normal Mode, 16 in Tall-Sprite-Mode)
// The amount of sprites already stored in the OAM Buffer must be less than 10
void Ppu::oam_scan(uint8_t screen_y)
{
    constexpr int TOTAL_OAM_ENTRIES = (OAM_END - OAM_START + 1) / static_cast<uint16_t>(sizeof(GBSprite));
    
    oam_buffer.clear();

    uint8_t obj_height = check_lcdc(LCDC::ObjSize) ? 16 : 8;

    for (int i = 0; i < TOTAL_OAM_ENTRIES; ++i)
    {
        if (oam_buffer.size() >= 10) break;

        uint16_t start_addr = OAM_START + static_cast<uint16_t>(sizeof(GBSprite)) * i;

        uint8_t y_pos = mmu.read_byte(start_addr);
        uint8_t x_pos = mmu.read_byte(start_addr + 1);
        if (x_pos == 0) continue;

        int obj_screen_y = static_cast<int>(y_pos) - 16;
        if (screen_y < obj_screen_y) continue;
        if (screen_y >= obj_screen_y + obj_height) continue;

        uint8_t tile_num = mmu.read_byte(start_addr + 2);
        uint8_t attrib = mmu.read_byte(start_addr + 3);

        oam_buffer.emplace_back(y_pos, x_pos, tile_num, attrib); 
    }

    std::stable_sort(
        oam_buffer.begin(), 
        oam_buffer.end(), 
        [](const GBSprite& s1, const GBSprite& s2)
        {
            return s1.x_pos >= s2.x_pos;
        }
    );
}

std::pair<uint8_t, uint8_t> Ppu::fetch_tile_row(int tile_map_x, int tile_map_y, bool use_9C00_tile_map) const
{ 
    int tile_x = (tile_map_x / GBTile::SIZE_PIXELS) & 0x1F;
    int tile_y = (tile_map_y / GBTile::SIZE_PIXELS) & 0x1F;
    int tile_id_offset = tile_x + (GBResolution::TILES_PER_ROW * tile_y); 

    uint16_t tile_map_start = use_9C00_tile_map ? TILE_MAP_9C00_START : TILE_MAP_9800_START;
    uint8_t tile_id = mmu.read_byte(tile_map_start + tile_id_offset);

    // Points to first row of tile
    uint16_t tile_address = check_lcdc(LCDC::BgWindowTileDataArea) ?
        TILE_DATA_ADDR0_START + (tile_id * GBTile::BYTES_PER_TILE) : 
        TILE_DATA_ADDR1_START + (static_cast<int8_t>(tile_id) * GBTile::BYTES_PER_TILE);
    
    // Points to correct row of tile
    uint16_t row_address = tile_address
        + ((tile_map_y % GBTile::SIZE_PIXELS) * GBTile::BYTES_PER_ROW);

    // Get first two bytes of tile data to obtain one tile row
    uint8_t tile_row_first_byte = mmu.read_byte(row_address);
    uint8_t tile_row_second_byte = mmu.read_byte(row_address + 1);

    return std::make_pair(tile_row_first_byte, tile_row_second_byte);
}

/// @note Used specifically for sprites
std::pair<uint8_t, uint8_t> Ppu::fetch_sprite_tile_row(int tile_id, int tile_map_y) const 
{    
    uint16_t row_address = TILE_DATA_ADDR0_START // Sprites always use 8000 method
        + (tile_id * GBTile::BYTES_PER_TILE) // Points to first row of tile
        + (tile_map_y * GBTile::BYTES_PER_ROW); // Points to right tile row

    // Get first two bytes of tile data to obtain one tile row
    uint8_t tile_row_first_byte = mmu.read_byte(row_address);
    uint8_t tile_row_second_byte = mmu.read_byte(row_address + 1);

    return std::make_pair(tile_row_first_byte, tile_row_second_byte);
}

std::array<uint8_t, 8> Ppu::decode_tile_row(uint8_t hi_byte, uint8_t lo_byte)
{
    std::array<uint8_t, 8> pixels{};

    // Starts from right pixel to left
    for (int i = 7; i >= 0; --i)
    {
        uint8_t msb = (lo_byte >> i) & 0x01;
        uint8_t lsb = (hi_byte >> i) & 0x01;
        uint8_t colour_id = (msb << 1) | lsb;

        pixels.at(i) = colour_id;
    }

    return pixels;
}

void Ppu::write_pixels(std::array<uint8_t, 8>& tile_pixels, int screen_x, int screen_y, std::array<uint8_t, 4>& palette)
{
    if (screen_y < 0 || screen_y >= GBResolution::HEIGHT) return; 

    // Starts from right pixel to left
    for (int i = 0; i < GBTile::SIZE_PIXELS; ++i)
    {
        int pixel_screen_x = screen_x + i;
        
        if (pixel_screen_x >= GBResolution::WIDTH) break;
        else if (pixel_screen_x < 0) continue;

        int buffer_index = (GBResolution::WIDTH * screen_y) + pixel_screen_x;
        int bit_index = 7 - i;        

        uint8_t raw_color_idx = tile_pixels.at(bit_index);
        scanline_buffer.at(pixel_screen_x) = raw_color_idx;

        uint8_t palette_color = palette.at(raw_color_idx);

        frame_buffer.at(buffer_index) = get_tile_colour(palette_color);
    }
}

void Ppu::write_sprite_pixels(std::array<uint8_t, 8>& tile_pixels, int screen_x, int screen_y, const GBSprite& sprite, std::array<uint8_t, 4>& palette)
{
    if (screen_y < 0 || screen_y >= GBResolution::HEIGHT) return; 

    // Starts from right pixel to left
    for (int i = 0; i < GBTile::SIZE_PIXELS; ++i)
    {
        int pixel_screen_x = screen_x + i;
        
        if (pixel_screen_x >= GBResolution::WIDTH) break;
        else if (pixel_screen_x < 0) continue;

        int bit_index = (sprite.flip_x) ? i : 7 - i;
        uint8_t raw_colour_idx = tile_pixels.at(bit_index);

        if (raw_colour_idx == 0) continue;

        uint8_t bg_color = scanline_buffer.at(pixel_screen_x);

        if (sprite.bg_priority == 1 && bg_color != 0)
            continue;
        
        uint8_t palette_colour = palette.at(raw_colour_idx);
        
        int buffer_index = (GBResolution::WIDTH * screen_y) + pixel_screen_x;
        frame_buffer.at(buffer_index) = get_tile_colour(palette_colour);
    }
}

/* Whole-frame rendering methods (Debugging) */
void Ppu::render_bg_frame()
{
    for (int y = 0; y < GBResolution::HEIGHT; ++y) 
        render_bg_scanline(y);
}

void Ppu::render_window_frame()
{
    window_internal_scanline_y = 0;

    for (int y = 0; y < GBResolution::HEIGHT; ++y) 
        render_window_scanline(y);
}

void Ppu::render_sprites_frame()
{
    for (int y = 0; y < GBResolution::HEIGHT; ++y)
    {
        oam_scan(y);
        render_sprites_scanline(y);
    }
}

/* Fill Screen w/ One Colour */
void Ppu::reset_screen()
{
    std::fill(frame_buffer.begin(), frame_buffer.end(), GBColours::COLOUR_11);
}

void Ppu::fill_white_screen()
{
    std::fill(frame_buffer.begin(), frame_buffer.end(), GBColours::COLOUR_00);
}