#include "cartridge.hpp"

#include <fstream>
#include <filesystem>
#include <iostream>
#include <exception>

Cartridge::Cartridge(size_t rom_size, size_t ram_size)
 : rom(), ram()
{
    rom.reserve(rom_size);
    ram.reserve(ram_size);
}

bool nintendo_logo_check(uint8_t header[])
{
    const uint8_t nintendo_logo[NINTENDO_LOGO_LEN] = {
        0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 
        0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,  
        0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 
        0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,  
        0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 
        0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
    };

    for (int i = 0; i < NINTENDO_LOGO_LEN; ++i)
    {
        if (nintendo_logo[i] != header[NINTENDO_LOGO_START + i])
            return false;
    }

    return true;
}

bool perform_checksum(uint8_t header[])
{
    uint8_t checksum = 0;
    for (uint16_t i = TITLE_START; i <= ROM_VERSION; ++i) 
    {
        checksum -= header[i] - 1;
    }

    return checksum == header[HEADER_CHECKSUM];
}

// Cartridge get_cartridge(int cartridge_type, size_t rom_size, size_t ram_size)
// {
//     switch (cartridge_type)
//     {
//         case static_cast<int>(CartridgeType::RomOnly):
//             return Cartridge(rom_size, ram_size);
//
//         default:
//             throw std::runtime_error("Unimplemented Cartridge Type");
//             break;
//     } 
// }

// Assume ROM only Cartridge for now
// Will add support for MBCs soon enough
bool read_header(const char* path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);

    if (!file.is_open()) 
    {
        std::cerr << "File does not exist" << std::endl;
        return false;
    }

    size_t file_size = static_cast<size_t>(std::filesystem::file_size(path));
    if (file_size < HEADER_SIZE)
    {
        std::cerr << "File size is too small" << std::endl;
        return false;
    }

    uint8_t header[HEADER_SIZE]{ 0 };

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(header), sizeof(header) / sizeof(uint8_t));

	file.close();

    // if (!nintendo_logo_check(buffer))
    // {
    //     std::cerr << "Failed Nintendo Logo Check" << std::endl;
    //     return nullptr;
    // }

    // if (!perform_checksum(buffer))
    // {
    //     return nullptr;
    // }
    // size_t rom_size = 32 * (1 << header[ROM_SIZE]);
    
    return true;
}