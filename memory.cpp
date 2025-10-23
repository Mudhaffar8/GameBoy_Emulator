#include "memory.hpp"

#include <fstream>
#include <filesystem>
#include <iostream>

Memory::Memory()
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