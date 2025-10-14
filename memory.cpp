#include "memory.hpp"

#include <fstream>
#include <iostream>

Memory::Memory() :
    memory{0}
{}

bool Memory::load_cartridge(Cartridge& cartridge)
{
    return false;
}

bool Memory::load_rom(const char* path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);

    if (!file.is_open()) 
    {
        std::cerr << "File does not exist" << std::endl;
        return false;
    }

    if (file.gcount() > MEMORY_SIZE)
    {
        std::cerr << "File size is too large" << std::endl;
        return false;
    }

    std::vector<uint8_t> buffer;
    buffer.reserve(file.gcount());

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(buffer.data()), sizeof(buffer) / sizeof(uint8_t));

    memcpy(memory, buffer.data(), sizeof(buffer));

    return true;
}