#include <stdexcept>
#include <iostream>
#include <assert.h>
#include <algorithm>

#include "ppu.hpp"

Ppu::Ppu(Mmu& _mmu) :
    mmu(_mmu),
    lcdc(mmu.read_byte_ref(LCD_CONTROL)),
    lcd_status(mmu.read_byte_ref(LCD_STATUS))
{}

void Ppu::tick(uint32_t cycles)
{}
 
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

void Ppu::render_frame()
{
    for (int y = 0; y < GBResolution::HEIGHT; ++y) 
        render_scanline(y);
}

void Ppu::render_scanline(int screen_y)
{
    bool bg_window_enable = check_lcdc(LCDC::BgWindowEnable);
    if (bg_window_enable)
        render_bg_scanline(screen_y);
    
    bool obj_enable = check_lcdc(LCDC::ObjEnable);
    if (obj_enable)
        render_sprites_scanline(screen_y);

    bool window_enable = check_lcdc(LCDC::WindowEnable);
    if (bg_window_enable && window_enable)
        render_window_scanline(screen_y);
}

/// @brief 
/// @param y the y-position of the scanline in SCREEN coordinates
void Ppu::render_bg_scanline(int screen_y)
{
    int tile_map_y = (screen_y + bg_scroll_y) & 0xFF;

    // SCX mod 8 => pixels to discard from left
    // 8 - (SCX mod 8) => pixels to discard from right tile 
    int tile_offset_x = bg_scroll_x % GBTile::SIZE_PIXELS;
    
    //std::cout << "BG Scanline #" << screen_y << '\n';

    // 21 unique tiles will need to be rendered at most
    for (int tile_x = 0; tile_x < GBResolution::TILES_PER_ROW_VISIBLE_MAX; ++tile_x)
    {
        int screen_x = tile_x * GBTile::SIZE_PIXELS;
        int tile_map_x = (screen_x + bg_scroll_x) & 0xFF;
        std::pair<uint8_t, uint8_t> tile_row = fetch_tile_row(tile_map_x, tile_map_y, TileMaps::Background);

        // Get x-value of leftmost pixel on the tile in SCREEN coordinates
        int last_tile_screen_x = screen_x - tile_offset_x;
        decode_tile_row(tile_row.first, tile_row.second, last_tile_screen_x, screen_y); 
    }

    //std::cout << '\n';
}

void Ppu::render_window_scanline(int screen_y)
{
    if (screen_y < window_scroll_y || screen_y >= GBResolution::HEIGHT) return;

    // Key Difference: Window Layer does not loop
    int tile_map_y = screen_y - window_scroll_y;

    int total_scroll_x = window_scroll_x - 7;
    int visible_tiles = (GBResolution::WIDTH - total_scroll_x) / GBTile::SIZE_PIXELS + 1; 
    
    // int tile_offset_x = total_scroll_x % GBTile::SIZE_PIXELS;

    //std::cout << "Window Scanline #" << screen_y << '\n';

    // Given the fact the window does not loop, 
    // there could range from 0 to 21 tiles to render on the screen at any given scanline.
    // Need to calculate based on the window_scroll_x and window_scroll_y values.
    for (int tile_x = 0; tile_x < visible_tiles; ++tile_x)
    {
        int screen_x = tile_x * GBTile::SIZE_PIXELS;
        int tile_map_x = screen_x + total_scroll_x; // Window Layer does not loop
        std::pair<uint8_t, uint8_t> tile_row = fetch_tile_row(screen_x, tile_map_y, TileMaps::Window);

        decode_tile_row(tile_row.first, tile_row.second, tile_map_x, screen_y); 
    }

    //std::cout << '\n';
}

void Ppu::render_sprites_scanline(int screen_y)
{
    for (const GBSprite& sprite : oam_buffer)
    {
        int obj_screen_y = static_cast<int>(sprite.y_pos) - 16;
        int obj_screen_x = static_cast<int>(sprite.x_pos) - 8;

        int tile_row_y = screen_y - obj_screen_y; 
        std::cout << "Tile Row Y: " << tile_row_y << '\n';

        auto sprite_tile_row = fetch_tile_row(sprite.tile_number, tile_row_y);

        decode_tile_row(
            sprite_tile_row.first, 
            sprite_tile_row.second,
            obj_screen_x, 
            screen_y
        );
    }
}

// Sprite X-Position must be greater than 0
// LY + 16 must be greater than or equal to Sprite Y-Position
// LY + 16 must be less than Sprite Y-Position + Sprite Height (8 in Normal Mode, 16 in Tall-Sprite-Mode)
// The amount of sprites already stored in the OAM Buffer must be less than 10
void Ppu::oam_scan(int screen_y)
{
    constexpr int TOTAL_OAM_ENTRIES = (OAM_END - OAM_START + 1) / static_cast<uint16_t>(sizeof(GBSprite));
    
    oam_buffer.clear();
    
    for (int i = 0; i < TOTAL_OAM_ENTRIES; ++i)
    {
        if (oam_buffer.size() >= 10) break;

        uint16_t start_addr = OAM_START + static_cast<uint16_t>(sizeof(GBSprite)) * i;

        uint8_t y_pos = mmu.read_byte(start_addr);
        uint8_t x_pos = mmu.read_byte(start_addr + 1);
        if (x_pos == 0 || x_pos >= GBResolution::WIDTH + GBTile::SIZE_PIXELS) continue;

        uint8_t obj_height = check_lcdc(LCDC::ObjSize) ?  
            GBTile::HEIGHT_SPRITE_PIXELS : 
            GBTile::SIZE_PIXELS;

        int obj_screen_y = static_cast<int>(y_pos) - 16;
        //std::cout << "Y pos: " << +y_pos << '\n';
        if (screen_y < obj_screen_y) continue;
        if (screen_y > obj_screen_y + obj_height) continue;

        uint8_t tile_num = mmu.read_byte(start_addr + 2);
        //std::cout << "Tile #: " << +tile_num << '\n';
        uint8_t attrib = mmu.read_byte(start_addr + 3);
        //std::cout << "Attrib: " << +attrib << '\n';

        GBSprite sprite(y_pos, x_pos, tile_num, attrib);
        oam_buffer.emplace_back(sprite); 
    }

    std::sort(oam_buffer.begin(), oam_buffer.end());

    /*
    for (const GBSprite& sprite : oam_buffer)
        std::cout << sprite << ", ";
    */
    if (!oam_buffer.empty()) 
    {
        std::cout << "# objs @ " << screen_y << ", OAM Size: " << oam_buffer.size() << std::endl;
        
        for (const GBSprite& sprite : oam_buffer)
            std::cout << sprite << ", " << '\n';
    }
}

/// @brief 
/// @param hi_byte 
/// @param lo_byte 
/// @param screen_x the x-position of the leftmost pixel on the tile in SCREEN coordinates 
/// @param screen_y the y-position of the leftmost pixel on the tile in SCREEN coordinates 
void Ppu::decode_tile_row(uint8_t hi_byte, uint8_t lo_byte, int screen_x, int screen_y)
{
    if (screen_y < 0 || screen_y >= GBResolution::HEIGHT) 
    { std::cout << "Screen out of range: " << screen_y << std::endl; return; }

    // Starts from right pixel to left
    for (int i = 7; i >= 0; --i)
    {
        int pixel_screen_x = screen_x + i;
        
        if (pixel_screen_x >= GBResolution::WIDTH) continue;
        else if (pixel_screen_x < 0) break;

        std::cout << "Printing Pixel @ x: " 
            << pixel_screen_x << 
            " y: " << screen_y << " colour: ";
        uint8_t msb = (lo_byte >> i) & 0x01;
        uint8_t lsb = (hi_byte >> i) & 0x01;
        uint8_t colour_id = (msb << 1) | lsb;

        std::cout << +colour_id << '\n';

        int index = (GBResolution::WIDTH * screen_y) + pixel_screen_x;
        frame_buffer.at(index) = get_tile_colour(colour_id);
    }
}

std::pair<uint8_t, uint8_t> Ppu::fetch_tile_row(int tile_map_x, int tile_map_y, TileMaps layer) const
{ 
    // Assume Tile addressing mode one for now
    // Bit 4 of LCDC
    // Get tile ID on current tile being operated on
    // Only doing BG tile map for now

    int tile_x = (tile_map_x / GBTile::SIZE_PIXELS) & 0x1F;
    int tile_y = (tile_map_y / GBTile::SIZE_PIXELS) & 0x1F;
    int tile_id_offset = tile_x + (GBResolution::TILES_PER_ROW * tile_y); 

    uint16_t tile_map_start = layer == TileMaps::Background ? BG_TILE_MAP_START : WINDOW_TILE_MAP_START;

    uint8_t tile_id = mmu.read_byte(tile_map_start + tile_id_offset);

    uint16_t row_address = TILE_DATA_ADDR0_START  
        + (tile_id * GBTile::BYTES_PER_TILE) // Points to first row of tile
        + ((tile_map_y % GBTile::SIZE_PIXELS) * GBTile::BYTES_PER_ROW); // Points to right row of tile

    // Get first two bytes of tile data to obtain one tile row
    uint8_t tile_row_first_byte = mmu.read_byte(row_address);
    uint8_t tile_row_second_byte = mmu.read_byte(row_address + 1);

    return std::make_pair(tile_row_first_byte, tile_row_second_byte);
}

std::pair<uint8_t, uint8_t> Ppu::fetch_tile_row(int tile_id, int tile_map_y) const 
{
    tile_id &= 0x1F;
    tile_map_y &= 0x1F;
    
    uint16_t row_address = TILE_DATA_ADDR0_START  
        + (tile_id * GBTile::BYTES_PER_TILE) // Points to first row of tile
        + ((tile_map_y % GBTile::SIZE_PIXELS) * GBTile::BYTES_PER_ROW); // Points to right tile row

    // Get first two bytes of tile data to obtain one tile row
    uint8_t tile_row_first_byte = mmu.read_byte(row_address);
    uint8_t tile_row_second_byte = mmu.read_byte(row_address + 1);

    return std::make_pair(tile_row_first_byte, tile_row_second_byte);
}


void Ppu::render_bg_frame()
{
    for (int y = 0; y < GBResolution::HEIGHT; ++y) 
        render_bg_scanline(y);
}

void Ppu::render_window_frame()
{
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

void Ppu::reset_screen()
{
    std::fill(frame_buffer.begin(), frame_buffer.end(), GBColours::COLOUR_11);
}

void Ppu::fill_white_screen()
{
    std::fill(frame_buffer.begin(), frame_buffer.end(), GBColours::COLOUR_00);
}

void Ppu::update_ppu_mode(Mode new_mode)
{
    ppu_mode = new_mode;
    lcd_status &= (0xFC & static_cast<uint8_t>(new_mode));
}


void Ppu::render_tile(int tile_x, int tile_y)
{
    // 0x1F as the BG tile map is 32x32.
    tile_x &= 0x1F;
    tile_y &= 0x1F;

    int screen_x = GBTile::SIZE_PIXELS * tile_x;
    int screen_y = GBTile::SIZE_PIXELS * tile_y;

    for (int y = 0; y < GBTile::SIZE_PIXELS; ++y)
    { 
        int tile_map_x = screen_x;
        int tile_map_y = screen_y + y;

        std::pair<uint8_t, uint8_t> tile_row = fetch_tile_row(tile_map_x, tile_map_y, TileMaps::Background);
        decode_tile_row(tile_row.first, tile_row.second, tile_map_x, tile_map_y);
    }
}