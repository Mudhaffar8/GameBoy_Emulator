#include "memory.hpp"

#include <fstream>
#include <filesystem>
#include <iostream>

Mmu::Mmu()
{
    std::vector<uint8_t> tileset_tiles 
    {
        0xFC,0xFC,0xFE,0xFE,0xDC,0x24,0xDC,0x74,0xDC,0x74,0x3E,0xC2,0x82,0xFE,0xFE,0xFE, // Mario's face
        0x00,0x00,0x3C,0x3C,0x7E,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x7E,0x00,0x3C, // Number zero
        0x55,0x33,0xAA,0xCC,0x55,0x33,0xAA,0xCC,0x55,0x33,0xAA,0xCC,0x55,0x33,0xAA,0xCC, // Checkered pattern
        0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F, // Other Checkered Pattern
    };

    std::copy(tileset_tiles.begin(), tileset_tiles.end(), memory + TILE_DATA_ADDR0_START);

    std::fill(memory + BG_TILE_MAP_START, memory + BG_TILE_MAP_END, 0x03);

    memory[BG_TILE_MAP_START + 64] = 0x00; // Make tile Mario's face
    memory[BG_TILE_MAP_START + 8] = 0x02; // Make 8th tile checkered pattern
}

bool Mmu::load_cartridge(Cartridge& cartridge)
{
    return false;
}

bool Mmu::load_rom(const char* path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);

    if (!file.is_open()) 
    {
        std::cerr << "File does not exist" << std::endl;
        return false;
    }

    size_t file_size = static_cast<size_t>(std::filesystem::file_size(path));
    if (file_size > MEMORY_SIZE)
    {
        std::cerr << "File size is too large" << std::endl;
        return false;
    }

    std::vector<uint8_t> buffer;
    buffer.reserve(file_size);

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(buffer.data()), file_size);

    std::copy(buffer.begin(), buffer.end(), memory);

    return true;
}