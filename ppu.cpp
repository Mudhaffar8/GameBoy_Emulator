#include <stdexcept>
#include <iostream>
#include <assert.h>

#include "ppu.hpp"

Ppu::Ppu(Mmu& _mmu) :
    mmu(_mmu)
{}

void Ppu::test()
{}

void Ppu::tick(uint32_t cycles)
{}
 
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
    for (int tile_x = 0; tile_x < GBResolution::TILES_PER_ROW_VISIBLE; ++tile_x)
    {
        int screen_x = tile_x * GBTile::SIZE_PIXELS;
        int bg_map_x = screen_x + scroll_x;
        int bg_map_y = screen_y + scroll_y;
        
        std::pair<uint8_t, uint8_t> tile_row = fetch_tile_row(bg_map_x, bg_map_y);
        decode_tile_row(tile_row.first, tile_row.second, bg_map_x, bg_map_y);
    }
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
        int bg_map_x = screen_x;
        int bg_map_y = screen_y + y;

        std::pair<uint8_t, uint8_t> tile_row = fetch_tile_row(bg_map_x, bg_map_y);
        decode_tile_row(tile_row.first, tile_row.second, bg_map_x, bg_map_y);
    }
}

void Ppu::decode_tile_row(uint8_t hi_byte, uint8_t lo_byte, int x, int y)
{   
    for (int i = 7; i >= 0; --i)
    {
        int pixel_x = x + i;
        if (pixel_x >= GBResolution::WIDTH) break;

        uint8_t msb = (lo_byte >> i) & 0x01;
        uint8_t lsb = (hi_byte >> i) & 0x01;

        frame_buffer.at((GBResolution::WIDTH * y) + pixel_x) = get_tile_colour((msb << 1) | lsb);
    }
}

std::pair<uint8_t, uint8_t> Ppu::fetch_tile_row(int bg_map_x, int bg_map_y) const
{ 
    // Assume Tile addressing mode one for now
    // Bit 4 of LCDC
    // Get tile ID on current tile being operated on
    // Only doing BG tile map for now

    int tile_row = bg_map_x / GBTile::SIZE_PIXELS;
    int tile_col = bg_map_y / GBTile::SIZE_PIXELS;
    int tile_id_offset = tile_row + (GBResolution::TILES_PER_ROW * tile_col); 

    uint8_t tile_id = mmu.read_byte(BG_TILE_MAP_START + tile_id_offset);

    uint16_t offset = TILE_DATA_ADDR0_START 
        + (tile_id * GBTile::BYTES_PER_TILE) 
        + ((bg_map_y % GBTile::SIZE_PIXELS) * GBTile::BYTES_PER_ROW);

    // Get first two bytes of tile data to obtain one tile row
    uint8_t tile_row_first_byte = mmu.read_byte(offset);
    uint8_t tile_row_second_byte = mmu.read_byte(offset + 1);

    return std::make_pair(tile_row_first_byte, tile_row_second_byte);
}