#include <stdexcept>
#include <iostream>
#include <assert.h>
#include <algorithm>

#include "ppu.hpp"

Ppu::Ppu(Mmu& _mmu) :
    mmu(_mmu)
{}

void Ppu::test()
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
    {
        render_bg_scanline(y);
        render_window_scanline(y);
    }
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
        std::pair<uint8_t, uint8_t> tile_row = fetch_tile_row(tile_map_x, tile_map_y, Layers::Background);

        // Get x-value of leftmost pixel on the tile in SCREEN coordinates
        int last_tile_screen_x = screen_x - tile_offset_x;
        decode_tile_row(tile_row.first, tile_row.second, last_tile_screen_x, screen_y); 
    }

    //std::cout << '\n';
}

void Ppu::render_window_scanline(int screen_y)
{
    // Key Difference: Window Layer does not loop
    int tile_map_y = screen_y + window_scroll_y;
    if (tile_map_y >= GBResolution::HEIGHT) return; 

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
        std::pair<uint8_t, uint8_t> tile_row = fetch_tile_row(screen_x, screen_y, Layers::Window);

        decode_tile_row(tile_row.first, tile_row.second, tile_map_x, tile_map_y); 
    }

    //std::cout << '\n';
}

/// @brief 
/// @param hi_byte 
/// @param lo_byte 
/// @param tile_start_x the x-position of the leftmost pixel on the tile in SCREEN coordinates 
/// @param tile_start_y the y-position of the leftmost pixel on the tile in SCREEN coordinates 
void Ppu::decode_tile_row(uint8_t hi_byte, uint8_t lo_byte, int tile_screen_x, int screen_y)
{
    if (screen_y < 0 || screen_y >= GBResolution::HEIGHT) return;

    //std::cout << "Tile Start at (" << tile_start_x << ", " << screen_y << ")\n";

    // Starts from right pixel to left
    for (int i = 7; i >= 0; --i)
    {
        int pixel_screen_x = tile_screen_x + i;
        
        if (pixel_screen_x >= GBResolution::WIDTH) continue;
        else if (pixel_screen_x < 0) break;

        //std::cout << "Printing Pixel x: " << pixel_screen_x << '\n';
        uint8_t msb = (lo_byte >> i) & 0x01;
        uint8_t lsb = (hi_byte >> i) & 0x01;
        uint8_t colour_id = (msb << 1) | lsb;

        //std::cout << +colour_id;

        int index = (GBResolution::WIDTH * screen_y) + pixel_screen_x;
        frame_buffer.at(index) = get_tile_colour(colour_id);
    }
}

std::pair<uint8_t, uint8_t> Ppu::fetch_tile_row(int tile_map_x, int tile_map_y, Layers layer) const
{ 
    // Assume Tile addressing mode one for now
    // Bit 4 of LCDC
    // Get tile ID on current tile being operated on
    // Only doing BG tile map for now

    int tile_x = (tile_map_x / GBTile::SIZE_PIXELS) & 0x1F;
    int tile_y = (tile_map_y / GBTile::SIZE_PIXELS) & 0x1F;
    int tile_id_offset = tile_x + (GBResolution::TILES_PER_ROW * tile_y); 

    uint16_t tile_map_start = layer == Layers::Background ? BG_TILE_MAP_START : WINDOW_TILE_MAP_START;

    uint8_t tile_id = mmu.read_byte(tile_map_start + tile_id_offset);

    uint16_t row_address = TILE_DATA_ADDR0_START  
        + (tile_id * GBTile::BYTES_PER_TILE) 
        + ((tile_map_y % GBTile::SIZE_PIXELS) * GBTile::BYTES_PER_ROW);

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

void Ppu::reset_screen()
{
    std::fill(frame_buffer.begin(), frame_buffer.end(), GBColours::COLOUR_11);
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

        std::pair<uint8_t, uint8_t> tile_row = fetch_tile_row(tile_map_x, tile_map_y, Layers::Background);
        decode_tile_row(tile_row.first, tile_row.second, tile_map_x, tile_map_y);
    }
}