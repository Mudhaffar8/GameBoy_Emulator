#include "cartridge.hpp"

#include <fstream>
#include <iostream>

static const uint16_t nintendo_logo[NINTENDO_LOGO_LEN] = {
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 
    0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,  
    0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 
    0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,  
    0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 
    0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
};

// Assume ROM only Cartridge for now
// Will add support for MBCs soon enough
bool Cartridge::load_rom(const char* path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);

    if (!file.is_open()) 
    {
        std::cerr << "File does not exist" << std::endl;
        return false;
    }

    uint8_t* buffer = new uint8_t[0x8000];

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(buffer), sizeof(buffer) / sizeof(uint8_t));

    // if (file.gcount() > sizeof(memory)) 
    // {
    //     std::cerr << "File size is too large!" << std::endl;
    //     delete buffer;
    //     return false;
    // }


	file.close();
    delete buffer;
    
    return true;
}

/*
Checksum Code
uint8_t checksum = 0;
for (uint16_t addr = TITLE_START; address <= TITLE_END; ++addr) 
{
    checksum = checksum - rom[addr] - 1;
}
*/