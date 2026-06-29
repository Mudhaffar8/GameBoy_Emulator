#include <stdexcept>
#include <iostream>
#include <cassert>
#include <cstring>
#include <algorithm>

#include "ppu.hpp"

Ppu::Ppu(Mmu& _mmu) :
    mmu(_mmu),
    lcdc(mmu.get_io_reg(LCD_CONTROL)),
    lcd_status(mmu.get_io_reg(LCD_STATUS)),
    ly_compare(mmu.get_io_reg(LY_COMPARE)),
    bg_palette(mmu.get_io_reg(BG_PALETTE)),
    obj0_palette(mmu.get_io_reg(OBJ_PALETTE_0)),
    obj1_palette(mmu.get_io_reg(OBJ_PALETTE_1)),
    bg_scroll_x(mmu.get_io_reg(VIEWPORT_X_POS)),
    bg_scroll_y(mmu.get_io_reg(VIEWPORT_Y_POS)),
    window_scroll_x(mmu.get_io_reg(WINDOW_X_POS)),
    window_scroll_y(mmu.get_io_reg(WINDOW_Y_POS)),
    scanline_y(mmu.get_io_reg(LCD_Y_COORDINATE))
{
    oam_buffer.reserve(10);
}

// While the PPU is accessing some video-related memory, 
// that memory is inaccessible to the CPU (writes are ignored, and reads return garbage values, usually $FF).
void Ppu::tick(uint32_t cycles)
{
    cycles_elapsed += cycles;

    /// @todo The screen should be blank on the first frame when LCD is turned ON
    if (cycles_elapsed >= GBTiming::CYCLES_PER_FRAME)
    {
        trigger_redisplay = true;
        cycles_elapsed = 0;
    }
    if (check_lcdc(LCDC::LCDPpuEnable) != lcd_was_on)
    {
        lcd_was_on = check_lcdc(LCDC::LCDPpuEnable);
        trigger_redisplay = true;

        if (!check_lcdc(LCDC::LCDPpuEnable))
        {
            std::cout << "LCD Turned OFF!\n";
            std::fill(frame_buffer.begin(), frame_buffer.end(), GBColours::COLOUR_00);

            set_scanline(0);
            GBInterrupts::unset_interrupt(mmu, Interrupts::LCD);
            window_internal_scanline_y = 0;

            cycles_elapsed = 0;
            ppu_mode = Mode::OamScan;
            lcd_status = (lcd_status & 0xFC) | static_cast<uint8_t>(Mode::HBlank);
            set_lcd_status(LCDStatus::Coincidence, false);

            return;
        }
        else 
            std::cout << "LCD Turned ON!\n";
    }
    if (!check_lcdc(LCDC::LCDPpuEnable))
        return;

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
            // Maybe add code for HDMA Transfer here?

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
        trigger_redisplay = true;

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
    refresh_palettes();

    std::memset(scanline_buffer.data(), 0x00, scanline_buffer.size());

    render_bg_scanline(screen_y);

    if (check_lcdc(LCDC::WindowEnable))
        render_window_scanline(screen_y);

    if (check_lcdc(LCDC::ObjEnable))
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

    // 21 unique tiles will need to be rendered at most
    for (int tile_x = 0; tile_x < GBResolution::TILES_PER_ROW_VISIBLE_MAX; ++tile_x)
    {        
        int screen_x = tile_x * GBTile::SIZE_PIXELS;
        int tile_map_x = (screen_x + bg_scroll_x) & 0xFF;

        uint8_t bg_map_attributes = fetch_bg_map_attributes(tile_map_x, tile_map_y, use_9C00_tile_map);
        
        bool flip_y = check_bg_map_attrib(BGMapAttributes::YFilp, bg_map_attributes);
        bool fetch_vram_1 = check_bg_map_attrib(BGMapAttributes::VRamBank, bg_map_attributes);
        auto tile_row = fetch_tile_row(tile_map_x, tile_map_y, use_9C00_tile_map, flip_y, fetch_vram_1);

        // Get x-value of leftmost pixel on the tile in SCREEN coordinates
        int last_tile_screen_x = screen_x - tile_offset_x;
        auto tile_pixels = decode_tile_row(tile_row.first, tile_row.second); 

        // Get palette and flip_x attributes 
        uint8_t palette_idx = bg_map_attributes & static_cast<uint8_t>(BGMapAttributes::ColourPalette);
        auto& palette = bg_palettes.at(palette_idx);

        bool flip_x = check_bg_map_attrib(BGMapAttributes::XFlip, bg_map_attributes);
        bool bg_priority = check_bg_map_attrib(BGMapAttributes::Priority, bg_map_attributes);
        write_pixels(tile_pixels, palette, last_tile_screen_x, screen_y, flip_x, bg_priority);
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

    // Given the fact the window does not loop, 
    // there could range from 0 to 21 tiles to render on the screen at any given scanline.
    // Need to calculate based on the window_scroll_x and window_scroll_y values.
    for (int tile_x = 0; tile_x < visible_tiles; ++tile_x)
    {
        int screen_x = tile_x * GBTile::SIZE_PIXELS;
        int tile_map_x = screen_x + total_scroll_x; // Window Layer does not loop

        uint8_t bg_map_attributes = fetch_bg_map_attributes(screen_x, window_internal_scanline_y, use_9C00_tile_map);

        bool flip_y = check_bg_map_attrib(BGMapAttributes::YFilp, bg_map_attributes);
        bool fetch_vram_1 = check_bg_map_attrib(BGMapAttributes::VRamBank, bg_map_attributes);
        auto tile_row = fetch_tile_row(screen_x, window_internal_scanline_y, use_9C00_tile_map, flip_y, fetch_vram_1);

        auto tile_pixels = decode_tile_row(tile_row.first, tile_row.second); 

        uint8_t palette_idx = bg_map_attributes & static_cast<uint8_t>(BGMapAttributes::ColourPalette);
        auto& palette = bg_palettes.at(palette_idx);    

        bool flip_x = check_bg_map_attrib(BGMapAttributes::XFlip, bg_map_attributes);
        bool bg_priority = check_bg_map_attrib(BGMapAttributes::Priority, bg_map_attributes);
        write_pixels(tile_pixels, palette, tile_map_x, screen_y, flip_x, bg_priority);
    }

    ++window_internal_scanline_y;
}

void Ppu::render_sprites_scanline(uint8_t screen_y)
{
    bool is_8x16 = check_lcdc(LCDC::ObjSize);
    int max_obj_height_idx = is_8x16 ? 15 : 7;

    for (auto sprite = oam_buffer.rbegin(); sprite != oam_buffer.rend(); ++sprite)
    {
        // Convert sprite position to screen coordinates
        int obj_screen_y = static_cast<int>(sprite->y_pos) - 16;
        int obj_screen_x = static_cast<int>(sprite->x_pos) - 8;

        int tile_row_y = (sprite->flip_y) ? 
            max_obj_height_idx - (screen_y - obj_screen_y) : 
            screen_y - obj_screen_y; 
        
        int tile_num = (is_8x16) ? (sprite->tile_number & 0xFE) : sprite->tile_number;
        bool fetch_vram_bank_1 = (sprite->vram_bank == 1);
        
        auto tile_row = fetch_sprite_tile_row(tile_num, tile_row_y, fetch_vram_bank_1);
        auto tile_pixels = decode_tile_row(tile_row.first, tile_row.second);

        auto& palette = obj_palettes.at(sprite->cgb_palette);
        write_sprite_pixels(tile_pixels, palette, *sprite, obj_screen_x, screen_y);
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
}

std::pair<uint8_t, uint8_t> Ppu::fetch_tile_row(int tile_map_x, int tile_map_y, bool use_9C00_tile_map, bool flip_y, bool fetch_vram_1) const
{ 
    int tile_x = (tile_map_x / GBTile::SIZE_PIXELS) & 0x1F;
    int tile_y = (tile_map_y / GBTile::SIZE_PIXELS) & 0x1F;
    int tile_id_offset = tile_x + (GBResolution::TILES_PER_ROW * tile_y); 

    uint16_t tile_map_start = use_9C00_tile_map ? TILE_MAP_9C00_START : TILE_MAP_9800_START;
    uint8_t tile_id = mmu.read_vram_0(tile_map_start + tile_id_offset);

    // Points to first row of tile
    uint16_t tile_address = check_lcdc(LCDC::BgWindowTileDataArea) ?
        TILE_DATA_ADDR0_START + (tile_id * GBTile::BYTES_PER_TILE) : 
        TILE_DATA_ADDR1_START + (static_cast<int8_t>(tile_id) * GBTile::BYTES_PER_TILE);
    
    uint8_t y = (flip_y) ? 
        7 - (tile_map_y % GBTile::SIZE_PIXELS) : 
        (tile_map_y % GBTile::SIZE_PIXELS);
    
    // Points to correct row of tile
    uint16_t row_address = tile_address
        + (y * GBTile::BYTES_PER_ROW);

    // Get first two bytes of tile data to obtain one tile row
    uint8_t tile_row_first_byte{}, tile_row_second_byte{};
    if (fetch_vram_1)
    {
        tile_row_first_byte = mmu.read_vram_1(row_address);
        tile_row_second_byte = mmu.read_vram_1(row_address + 1);
    }
    else 
    {
        tile_row_first_byte = mmu.read_vram_0(row_address);
        tile_row_second_byte = mmu.read_vram_0(row_address + 1);
    }

    return std::make_pair(tile_row_first_byte, tile_row_second_byte);
}

/// @note Used specifically for sprites
std::pair<uint8_t, uint8_t> Ppu::fetch_sprite_tile_row(int tile_id, int tile_map_y, bool fetch_vram_bank_1) const 
{    
    uint16_t row_address = TILE_DATA_ADDR0_START // Sprites always use 8000 method
        + (tile_id * GBTile::BYTES_PER_TILE) // Points to first row of tile
        + (tile_map_y * GBTile::BYTES_PER_ROW); // Points to right tile row

    // Get first two bytes of tile data to obtain one tile row
    uint8_t tile_row_first_byte{}, tile_row_second_byte{};
    if (fetch_vram_bank_1)
    {
        tile_row_first_byte = mmu.read_vram_1(row_address);
        tile_row_second_byte = mmu.read_vram_1(row_address + 1);
    }
    else 
    {
        tile_row_first_byte = mmu.read_vram_0(row_address);
        tile_row_second_byte = mmu.read_vram_0(row_address + 1);
    }
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

void Ppu::write_pixels(std::array<uint8_t, 8>& tile_pixels, std::array<uint8_t, 8>& palette, int screen_x, int screen_y, bool flip_x, bool bg_priority)
{
    if (screen_y < 0 || screen_y >= GBResolution::HEIGHT) return; 

    // Starts from right pixel to left
    for (int i = 0; i < GBTile::SIZE_PIXELS; ++i)
    {
        int pixel_screen_x = screen_x + i;
        
        if (pixel_screen_x >= GBResolution::WIDTH) break;
        else if (pixel_screen_x < 0) continue;

        int buffer_index = (GBResolution::WIDTH * screen_y) + pixel_screen_x;
        int bit_index = (flip_x) ? i : 7 - i;

        uint8_t raw_colour_idx = tile_pixels.at(bit_index);
        scanline_buffer.at(pixel_screen_x) = raw_colour_idx;
        if (bg_priority) 
            scanline_buffer.at(pixel_screen_x) |= 0x80; // How did I pass cgb-acid2 before lol

        uint8_t palette_colour_low = palette.at(raw_colour_idx * 2);
        uint8_t palette_colour_high = palette.at(raw_colour_idx * 2 + 1);

        frame_buffer.at(buffer_index) = convert_rgb555_to_rgba32(palette_colour_high, palette_colour_low);
    }
}

void Ppu::write_sprite_pixels(std::array<uint8_t, 8>& tile_pixels, std::array<uint8_t, 8>& palette, const GBSprite& sprite, int screen_x, int screen_y)
{
    if (screen_y < 0 || screen_y >= GBResolution::HEIGHT) return; 

    bool bg_window_enable = check_lcdc(LCDC::BgWindowEnable);

    // Starts from right pixel to left
    for (int i = 0; i < GBTile::SIZE_PIXELS; ++i)
    {
        int pixel_screen_x = screen_x + i;
        
        if (pixel_screen_x >= GBResolution::WIDTH) break;
        else if (pixel_screen_x < 0) continue;

        int bit_index = (sprite.flip_x) ? i : 7 - i;
        uint8_t raw_colour_idx = tile_pixels.at(bit_index);

        if (raw_colour_idx == 0) continue;

        uint8_t bg_pixel = scanline_buffer.at(pixel_screen_x);
        uint8_t bg_color_idx = bg_pixel & 0x3;
        bool bg_priority = bg_pixel & 0x80;

        if (bg_color_idx != 0 && bg_window_enable && !(bg_priority == 0 && sprite.bg_priority == 0))
            continue;
        
        uint8_t palette_colour_low = palette.at(raw_colour_idx * 2);
        uint8_t palette_colour_high = palette.at(raw_colour_idx * 2 + 1);

        int buffer_index = (GBResolution::WIDTH * screen_y) + pixel_screen_x;
        frame_buffer.at(buffer_index) = convert_rgb555_to_rgba32(palette_colour_high, palette_colour_low);
    }
}

/* Game Boy Color Helper Methods */
uint32_t Ppu::convert_rgb555_to_rgba32(uint8_t high_byte, uint8_t low_byte)
{
    uint8_t r5 = low_byte & 0x1F;
    uint8_t g5 = ((high_byte & 3) << 3) | ((low_byte & 0xE0) >> 5);
    uint8_t b5 = (high_byte & 0x7C) >> 2;

    uint8_t r8 = (r5 << 3) | (r5 >> 2);
    uint8_t g8 = (g5 << 3) | (g5 >> 2);
    uint8_t b8 = (b5 << 3) | (b5 >> 2);

    return (r8 << 24) | (g8 << 16) | (b8 << 8) | 0xFF;
}

/* Palette Methods */
std::array<uint8_t, 8> Ppu::get_cgb_palette(std::array<uint8_t, 64>& cram_palette, uint8_t palette_num)
{
    std::array<uint8_t, 8> palette{};
    for (int i = 0; i < palette.size(); ++i)
        palette.at(i) = cram_palette.at(GBPalette::BYTES_PER_PALETTE * palette_num + i);

    return palette;
}

void Ppu::refresh_palettes()
{
    auto& obj_cram = mmu.get_obj_cram();
    auto& bg_cram = mmu.get_bg_cram();
    for (int i = 0; i < bg_palettes.size(); ++i)
    {
        obj_palettes.at(i) = get_cgb_palette(obj_cram, i);
        bg_palettes.at(i) = get_cgb_palette(bg_cram, i);
    }
}

uint8_t Ppu::fetch_bg_map_attributes(int tile_map_x, int tile_map_y, bool use_9C00_tile_map)
{
    // I'll optimize this later
    int tile_x = (tile_map_x / GBTile::SIZE_PIXELS) & 0x1F;
    int tile_y = (tile_map_y / GBTile::SIZE_PIXELS) & 0x1F;
    int tile_id_offset = tile_x + (GBResolution::TILES_PER_ROW * tile_y); 

    uint16_t tile_attrib_start = (use_9C00_tile_map) ? TILE_MAP_9C00_START : TILE_MAP_9800_START;
    return mmu.read_vram_1(tile_attrib_start + tile_id_offset);
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