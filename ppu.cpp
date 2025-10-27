#include <stdexcept>
#include <iostream>

#include "ppu.hpp"

Ppu::Ppu(Mmu* _mmu) :
    mmu(_mmu)
{}

void Ppu::test()
{
    
}

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
    for (int x = 0; x < 20; ++x)
    {
        for (int y = 0; y < 8; ++y)
        {
            for (tile_map_i = x * 20; tile_map_i < (x * 20) + 20; ++tile_map_i)
            {
                std::pair<uint8_t, uint8_t> tile_row = fetch_tile_row(y);
                decode_tile_row(tile_row.first, tile_row.second, tile_map_i, y);
            }
        }
    }
}

void Ppu::render_scanline()
{
    for (tile_map_i = 0; tile_map_i < 20; ++tile_map_i)
    {
        std::pair<uint8_t, uint8_t> tile_row = fetch_tile_row(0);
        decode_tile_row(tile_row.first, tile_row.second, tile_map_i, 0);
    }
}

void Ppu::render_tile()
{
    for (int y = 0; y < 8; ++y)
    { 
        std::pair<uint8_t, uint8_t> tile_row = fetch_tile_row(y);
        decode_tile_row(tile_row.first, tile_row.second, 0, y);
    }
}


void Ppu::decode_tile_row(uint8_t hi_byte, uint8_t lo_byte, int x, int y)
{   
    for (int i = 7; i >= 0; --i)
    {
        uint8_t msb = (lo_byte >> i) & 0x01;
        uint8_t lsb = (hi_byte >> i) & 0x01;

        // std::cout << ((GBResolution::WIDTH) * y + (x * 8) + i) << ", ";

        frame_buffer[(GBResolution::WIDTH) * y + (x * 8) + i] = get_tile_colour((msb << 1) | lsb);
    }

    // std::cout << '\n';
}

/**
 * @param tile_row_i - Rep. which row of a specific tile to fetch (from 0 to 7)
 * @returns the first and second byte of a tile representing one tile row
 */
std::pair<uint8_t, uint8_t> Ppu::fetch_tile_row(int tile_row_i)
{
    // Assume Tile addressing mode one for now
    // Bit 4 of LCDC

    // Get tile ID on current tile being operated on
    // Only doing BG tile map for now
    uint8_t tile_id = mmu->read_byte(BG_TILE_MAP_START + tile_map_i);

    uint16_t offset = TILE_DATA_ADDR0_START + (tile_id * 16) + (tile_row_i * 2);
    // int offset = TILE_DATA_ADDR1_START + (static_cast<int8_t>(tile_number) * 16);

    // Get first two bytes of tile data to obtain one tile row
    uint8_t tile_line_first_byte = mmu->read_byte(offset);
    uint8_t tile_line_second_byte = mmu->read_byte(offset + 1);

    return std::make_pair(tile_line_first_byte, tile_line_second_byte);
}