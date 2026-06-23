#include "cartridge.hpp"

#include <fstream>
#include <filesystem>
#include <exception>
#include <algorithm>

Cartridge::Cartridge(std::vector<uint8_t>& rom_data, std::vector<uint8_t>& ram_data) :
    rom(std::move(rom_data)),
    ram(std::move(ram_data))
{
    print();
}

void Cartridge::print() 
{
    std::cout << "Cartridge Type: " << +rom.at(CARTRIDGE_TYPE) << '\n'
        << "ROM Size: " << rom.size() << " (Total ROM Banks: " << +get_num_rom_banks() << ")\n"
        << "RAM Size: " << ram.size() << " (Total RAM Banks: " << +get_num_ram_banks() << ")\n";
}

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
std::unique_ptr<Cartridge> Cartridge::load_rom(const std::string path)
{
    // Validate file path
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) 
    {
        std::cerr << "File does not exist" << std::endl;
        return nullptr;
    }

    size_t file_size = static_cast<size_t>(std::filesystem::file_size(path));
    if (file_size < 0)
    {
        std::cerr << "File size is too small" << std::endl;
        return nullptr;
    }

    // Validate Header Data
    std::array<uint8_t, HEADER_SIZE> header{};
    file.read(reinterpret_cast<char*>(header.data()), header.size());

    if (!nintendo_logo_check(header))
    {
        std::cerr << "Failed Nintendo Logo Check" << std::endl;
        return nullptr;
    }
    // nvm I just wrote it wrong
    if (!perform_checksum(header))
    {
        std::cerr << "Failed Check Sum" << std::endl;
        return nullptr;
    }

    // Construct Cartridge
    uint8_t cartridge_type = header.at(CARTRIDGE_TYPE);

    // Both in KBs
    size_t rom_size = Cartridge::get_rom_size(header.at(ROM_SIZE));
    size_t ram_size = Cartridge::get_ram_size(header.at(RAM_SIZE));
    
    std::vector<uint8_t> rom_data(rom_size, 0);
    std::vector<uint8_t> ram_data(ram_size, 0);

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(rom_data.data()), rom_data.size());
    file.close();

    auto cartridge = std::make_unique<Cartridge>(rom_data, ram_data);

    return cartridge;
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

void Cartridge::memory_write(uint8_t byte, uint16_t address)
{
    uint8_t cartridge_type = rom.at(CARTRIDGE_TYPE);
    switch (cartridge_type)
    {
        // Some ROM-only test files have cartridge type set to MBC1?
        case Type::RomOnly:
            if (ram.size() > 0 && address > 0x9FFF && address < 0xC000)
                ram.at(address) = byte;

        case Type::MBC1:
        case Type::MBC1Ram:
        case Type::MBC1RamBattery:
            mbc1_write(byte, address);
            break;
            
        default:
            std::cout << "Uknown Cartridge Type: " << +cartridge_type << '\n';
            break;
    } 
}

uint8_t Cartridge::memory_read(uint16_t address)
{
    uint8_t cartridge_type = rom.at(CARTRIDGE_TYPE);
    switch (cartridge_type)
    {
        // Some ROM-only test files have cartridge type set to MBC1?
        case Type::RomOnly:
        //case Type::MBC1:
            if (ram.size() > 0 && address > 0x9FFF && address < 0xC000)
                return ram.at(address);

            return rom.at(address);
        
        case Type::MBC1:
        case Type::MBC1Ram:
        case Type::MBC1RamBattery:
            return mbc1_read(address);
            
        default:
            std::cout << "Unknown Cartridge Type: " << +cartridge_type << '\n';
            return 0x00;
    } 
}

void Cartridge::mbc1_write(uint8_t byte, uint16_t address)
{
    switch (address & 0xF000)
    {
    case 0x0000:
    case 0x1000:
        if ((byte & 0xF) == 0xA) 
            external_ram_enable = true;
        else 
            external_ram_enable = false;
        break;

    // Sets ROM Bank Number
    case 0x2000:
    case 0x3000:
        {
            if (byte == 0) { rom_bank_number = 1; return; }
            if (byte == 20) { rom_bank_number = 21; return; }
            if (byte == 40) { rom_bank_number = 41; return; }
            if (byte == 60) { rom_bank_number = 61; return; }

            uint8_t bitmask = std::min(0x1F, get_num_rom_banks() - 1);
            rom_bank_number = (byte & bitmask & 0x1F);
        }
        break;

    // Set RAM Bank Number
    case 0x4000:
    case 0x5000:
        ram_bank_number = (byte & 0x3);
        break;

    case 0x6000:
    case 0x7000:
        // Mode Select
        banking_mode = ((byte & 1) == 1);
        break;
    
    case 0xA000:
    case 0xB000:
        // Writing to External RAM
        if (external_ram_enable)
        {   
            if (ram.size() == (2 * ONE_KB) || ram.size() == (8 * ONE_KB))
            {
                ram.at((address - 0xA000) % ram.size()) = byte;
            }
            else if (ram.size() == (32 * ONE_KB))
            {
                if (banking_mode)
                    ram.at(0x2000 * ram_bank_number + (address - 0xA000)) = byte;
                else
                    ram.at(address - 0xA000) = byte;
            }
        }
        break;
    }
}

uint8_t Cartridge::mbc1_read(uint16_t address)
{
    switch (address & 0xF000)
    {
    case 0x0000:
    case 0x1000:
    case 0x2000:
    case 0x3000:
        if (!banking_mode)
            return rom.at(address);
        else
        {
            uint8_t total_rom_banks = get_num_rom_banks();
            uint16_t zero_bank_number = 0;

            if (total_rom_banks == 64) // Multi-Cart ROMS work differently (<< 4 instead)
                zero_bank_number = ((ram_bank_number & 1) << 5);
            else if (total_rom_banks == 128)
                zero_bank_number = ((ram_bank_number & 3) << 5);

            return rom.at(address + (0x4000 * zero_bank_number));
        }

    case 0x4000:
    case 0x5000:
    case 0x6000:
    case 0x7000:
        {
            uint8_t total_rom_banks = get_num_rom_banks();
            //  ROM Bank number ANDed with the bitmap of the corresponding ROM size
            uint8_t rom_size = total_rom_banks - 1;
            uint8_t high_bank_number = rom_bank_number & rom_size;

            if (total_rom_banks == 64) // Multi-Cart ROMS work differently
            {
                high_bank_number &= ~(1 << 5);
                high_bank_number |= ((ram_bank_number & 1) << 5);
            }
            else if (total_rom_banks == 128)
            {
                high_bank_number &= ~(3 << 5);
                high_bank_number |= ((ram_bank_number & 3) << 5);
            }

            return rom.at((0x4000 * high_bank_number) + (address - 0x4000));
        }
    
    case 0xA000:
    case 0xB000:
        // Writing to External RAM
        if (external_ram_enable)
        {   
            if (ram.size() == (2 * ONE_KB) || ram.size() == (8 * ONE_KB))
                return ram.at((address - 0xA000) % ram.size());
            else if (ram.size() == (32 * ONE_KB))
            {
                if (banking_mode)
                    return ram.at(0x2000 * ram_bank_number + (address - 0xA000));
                else
                    return ram.at(address - 0xA000);
            }
        }
        return 0xFF;
    }

    return 0x00;
}