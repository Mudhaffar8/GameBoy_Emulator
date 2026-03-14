#include <stdexcept>
#include <iostream>

#include "ppu.hpp"

Ppu::Ppu(Mmu& _mmu) :
    mmu(_mmu)
{}

void Ppu::test()
{}

void Ppu::tick(uint32_t cycles)
{}

uint32_t Ppu::get_tile_colour(uint8_t bit2)
{
    switch(bit2)
    {
    case 0b00:
        return GBColours::COLOUR00;
    case 0b01:
        return GBColours::COLOUR01;
    case 0b10:
        return GBColours::COLOUR10;
    case 0b11:
        return GBColours::COLOUR11;
    default:
        throw std::invalid_argument("Invalid byte larger than 2: " + bit2);
    }
}

void Ppu::render_frame()
{
    for (int y = 0; y < GBResolution::HEIGHT; ++y) 
        render_scanline();
}

void Ppu::render_scanline()
{
    scanline_x = 0;
    //scroll_x = 0;
    // scroll_y = 8;

    for (int x = 0; x < GBResolution::NUM_OF_TILES; ++x)
    {
        int bg_map_x = scanline_x + scroll_x;
        int bg_map_y = scanline_y + scroll_y;
        
        std::pair<uint8_t, uint8_t> tile_row = fetch_tile_row(bg_map_x, bg_map_y);
        decode_tile_row(tile_row.first, tile_row.second, bg_map_x, bg_map_y);
    }

    ++scanline_y;
}

void Ppu::render_tile()
{
    for (; scanline_y < 8; ++scanline_y)
    { 
        int bg_map_x = scanline_x + scroll_x;
        int bg_map_y = scanline_y + scroll_y;

        std::pair<uint8_t, uint8_t> tile_row = fetch_tile_row(bg_map_x, bg_map_y);
        decode_tile_row(tile_row.first, tile_row.second, 0, bg_map_y);
    }
}


void Ppu::decode_tile_row(uint8_t hi_byte, uint8_t lo_byte, int x, int y)
{   
    int pixels_to_discard_start = x % 8;
    int calc = GBResolution::WIDTH - 1 - scanline_x < 8;
    int pixels_to_discard_end = (calc < 8) ? calc : 0;

    // for (int i = pixels_to_discard_start; i >= pixels_to_discard_end; --i)
    for (int i = 7; i >= 0; --i)
    {
        //if (scanline_x >= GBResolution::WIDTH) break;

        uint8_t msb = (lo_byte >> i) & 0x01;
        uint8_t lsb = (hi_byte >> i) & 0x01;

        // std::cout << ((GBResolution::WIDTH) * y + x + i) << " (";

        frame_buffer.at((GBResolution::WIDTH * y) + x + i) = get_tile_colour((msb << 1) | lsb);

        // std::cout << "(" << get_tile_colour((msb << 1) | lsb) << ") (" << y << ", " << x + i << ")";

        ++scanline_x;
    }
}

std::pair<uint8_t, uint8_t> Ppu::fetch_tile_row(int bg_map_x, int bg_map_y)
{ 
    // Assume Tile addressing mode one for now
    // Bit 4 of LCDC
    // Get tile ID on current tile being operated on
    // Only doing BG tile map for now

    // BG_TILE_MAP_START + (bg_map_y * GBResolution::WIDTH_WHOLE) + bg_map_x
    int tile_id_offset = (bg_map_x / 8) + (GBResolution::NUM_OF_TILES_WHOLE * (bg_map_y / 8)); 
    uint8_t tile_id = mmu.read_byte(BG_TILE_MAP_START + tile_id_offset);

    uint16_t offset = TILE_DATA_ADDR0_START + (tile_id * 16) + ((bg_map_y % 8) * 2);
    // int offset = TILE_DATA_ADDR1_START + (static_cast<int8_t>(tile_id) * 16);

    // Get first two bytes of tile data to obtain one tile row
    uint8_t tile_line_first_byte = mmu.read_byte(offset);
    uint8_t tile_line_second_byte = mmu.read_byte(offset + 1);

    return std::make_pair(tile_line_first_byte, tile_line_second_byte);
}