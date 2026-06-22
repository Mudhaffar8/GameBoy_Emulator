#include "cartridge.hpp"

#include <fstream>
#include <filesystem>
#include <exception>

constexpr int ONE_KB = 1024;

Cartridge::Cartridge(std::vector<uint8_t>& rom_data, std::vector<uint8_t>& ram_data) :
    rom(std::move(rom_data)),
    ram(std::move(ram_data))
{}

bool nintendo_logo_check(std::array<uint8_t, HEADER_SIZE>& header)
{

    for (int i = 0; i < nintendo_logo.size(); ++i)
    {
        if (nintendo_logo.at(i) != header.at(NINTENDO_LOGO_START + i))
            return false;
    }

    return true;
}

bool perform_checksum(std::array<uint8_t, HEADER_SIZE>& header)
{
    uint8_t checksum = 0;
    for (uint16_t i = 0x134; i <= 0x14C; ++i) 
    {
        checksum = checksum - header.at(i) - 1;
    }

    return checksum == header.at(HEADER_CHECKSUM);
}

// Assume ROM-only Cartridge for now
// Will add support for MBCs soon enough
std::optional<Cartridge> Cartridge::load_rom(const std::string path)
{
    // Validate file path
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) 
    {
        std::cerr << "File does not exist" << std::endl;
        return std::nullopt;
    }

    size_t file_size = static_cast<size_t>(std::filesystem::file_size(path));
    if (file_size < 0)
    {
        std::cerr << "File size is too small" << std::endl;
        return std::nullopt;
    }

    // Validate Header Data
    std::array<uint8_t, HEADER_SIZE> header{};
    file.read(reinterpret_cast<char*>(header.data()), header.size());

    if (!nintendo_logo_check(header))
    {
        std::cerr << "Failed Nintendo Logo Check" << std::endl;
        return std::nullopt;
    }
    // nvm I just wrote it wrong
    if (!perform_checksum(header))
    {
        std::cerr << "Failed Check Sum" << std::endl;
        return std::nullopt;
    }

    // Construct Cartridge
    uint8_t cartridge_type = header.at(CARTRIDGE_TYPE);

    // Both in KBs
    size_t rom_size = Cartridge::get_rom_size(header.at(ROM_SIZE));
    size_t ram_size = Cartridge::get_ram_size(header.at(RAM_SIZE));
    
    std::vector<uint8_t> rom_data(rom_size, 0);

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(rom_data.data()), rom_data.size());
    file.close();

    auto cartridge = Cartridge::get_cartridge(rom_data, ram_size, cartridge_type);

    return cartridge;
}

std::optional<Cartridge> Cartridge::get_cartridge(std::vector<uint8_t>& rom_data, size_t ram_size, uint8_t cartridge_type)
{
    std::vector<uint8_t> ram_data(ram_size, 0);

    switch (cartridge_type)
    {
        // Some ROM-only test files have cartridge type set to MBC1?
        case Type::RomOnly:
            return std::make_optional<Cartridge>(rom_data, ram_data);

        default:
            std::cout << "Unimplemented Cartridge Type: " << +cartridge_type << '\n';
            return std::nullopt;
    } 
}

size_t Cartridge::get_rom_size(uint8_t rom_size)
{
    return (32 * ONE_KB) * (1 << rom_size);
}

size_t Cartridge::get_ram_size(uint8_t ram_size)
{
    switch(ram_size)
    {
    case RamSize::NoRam:
        return 0;
    case RamSize::Bank8K:
        return 8 * ONE_KB;
    case RamSize::Bank128K:
        return 128 * ONE_KB;
    case RamSize::Bank32K:
        return 32 * ONE_KB;
    case RamSize::Bank64K:
        return 64 * ONE_KB;
    default:
        std::cout << "Uknown RAM Size Type: " << +ram_size << '\n';
        return 0;
    }
}