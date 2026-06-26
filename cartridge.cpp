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
    std::cout << "Cartridge Type: " << std::hex << +rom.at(CARTRIDGE_TYPE) << '\n'
        << "ROM Size: " << std::dec << (rom.size() / ONE_KB) << "KB (Total ROM Banks: " << +get_num_rom_banks() << ")\n"
        << "RAM Size: " << std::dec << (ram.size() / ONE_KB) << "KB (Total RAM Banks: " << +get_num_ram_banks() << ")\n";
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
        case Type::RomRam:
        case Type::RomRamBattery:
            if (ram.size() > 0 && address > 0x9FFF && address < 0xC000)
                ram.at(address) = byte;
            break;

        case Type::MBC1:
        case Type::MBC1Ram:
        case Type::MBC1RamBattery:
            mbc1_write(byte, address);
            break;

        case Type::MBC3:
        case Type::MBC3TimerBattery:
        case Type::MBC3TimerRamBattery:
        case Type::MBC3Ram:
        case Type::MBC3RamBattery:
            mbc3_write(byte, address);
            break;
        
        case Type::MBC5:
        case Type::MBC5Ram:
        case Type::MBC5RamBattery:
            mbc5_write(byte, address);
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
        case Type::RomRam:
        case Type::RomRamBattery:
            if (ram.size() > 0 && address > 0x9FFF && address < 0xC000)
                return ram.at(address);

            return rom.at(address);
        
        case Type::MBC1:
        case Type::MBC1Ram:
        case Type::MBC1RamBattery:
            return mbc1_read(address);

        case Type::MBC3:
        case Type::MBC3TimerBattery:
        case Type::MBC3TimerRamBattery:
        case Type::MBC3Ram:
        case Type::MBC3RamBattery:
            return mbc3_read(address);

        case Type::MBC5:
        case Type::MBC5Ram:
        case Type::MBC5RamBattery:
            return mbc5_read(address);

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

            uint8_t bitmask = std::min(0x1F, get_num_rom_banks() - 1);
            rom_bank_number = (byte & bitmask);
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

void Cartridge::mbc3_write(uint8_t byte, uint16_t address)
{
    switch (address & 0xF000)
    {
        // Set External Ram Enable and RTC Registers
    case 0x0000:
    case 0x1000:
        if ((byte & 0xF) == 0xA) 
        {
            external_ram_enable = true;
            rtc_enable = true;
        }
        else 
            external_ram_enable = false;
        break;

    case 0x2000:
    case 0x3000:
        rom_bank_number = (byte == 0) ? 1 : (byte & 0x7F);
        //std::cout << "ROM Bank Number: " << rom_bank_number << '\n';
        break;
    
    case 0x4000:
    case 0x5000:
        if (byte <= 3)
        {
            ram_bank_number = byte;
            is_rtc_mapped_to_ram = false;
        }
        else if (byte >= 0x8 && byte <= 0xC)
        {
            rtc_register_id = byte - 0x8;
            is_rtc_mapped_to_ram = true;
        }
        break;
    
    // RTC Data Latch
    case 0x6000:
    case 0x7000:
        break;
    
    case 0xA000:
    case 0xB000:
        // Writing to External RAM
        if (!is_rtc_mapped_to_ram && external_ram_enable)
            ram.at(0x2000 * ram_bank_number + (address - 0xA000)) = byte;
        else
            write_to_rtc_register(byte);
        break;
    }
}

uint8_t Cartridge::mbc3_read(uint16_t address)
{
    switch (address & 0xF000)
    {
    case 0x0000:
    case 0x1000:
    case 0x2000:
    case 0x3000:
        return rom.at(address);
        break;

    case 0x4000:
    case 0x5000:
    case 0x6000:
    case 0x7000:
        return rom.at(0x4000 * rom_bank_number + (address - 0x4000));
        break;

    case 0xA000:
    case 0xB000:
        // Writing to External RAM
        if (is_rtc_mapped_to_ram)
            return read_to_rtc_register();
        else
        {   
            if (external_ram_enable)
                return ram.at(0x2000 * ram_bank_number + (address - 0xA000));
            else
                return 0xFF;
        }
    }

    return 0;
}

uint8_t Cartridge::read_to_rtc_register()
{
    switch(rtc_register_id)
    {
    case 0: return rtc_s.latch | ~RTC_S_BITMASK;
    case 1: return rtc_m.latch | ~RTC_M_BITMASK;
    case 2: return rtc_h.latch | ~RTC_H_BITMASK;
    case 3: return rtc_dl.latch;
    case 4: return rtc_dh.latch | ~RTC_DH_BITMASK;
    default: return 0xFF;
    }
}

void Cartridge::write_to_rtc_register(uint8_t byte)
{
    switch(rtc_register_id)
    {
    case 0:
        rtc_s.set_both(byte & RTC_S_BITMASK); 
        break;
    case 1:
        rtc_m.set_both(byte & RTC_M_BITMASK); 
        break;
    case 2:
        rtc_h.set_both(byte & RTC_H_BITMASK); 
        break;
    case 3:
        rtc_dl.set_both(byte); 
        break;
    case 4:
        rtc_dh.set_both(byte & RTC_DH_BITMASK);
        break;
    default:
        break;
    }
}

void Cartridge::mbc5_write(uint8_t byte, uint16_t address)
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

    // Sets ROM Bank Number Low 8-bits
    case 0x2000:
        rom_bank_number &= 0x100;
        rom_bank_number |= byte;
        //std::cout << "ROM Bank Number: " << +rom_bank_number << '\n'; 
        break;

    // Sets ROM Bank Number high bit
    case 0x3000:
        rom_bank_number &= 0xFF;
        rom_bank_number |= (byte & 1) << 8;
        //std::cout << "ROM Bank Number: " << +rom_bank_number << '\n'; 
        break;

    // Set RAM Bank Number
    case 0x4000:
    case 0x5000:
        {
            uint8_t bitmask = std::min(0xF, get_num_ram_banks() - 1);
            ram_bank_number = (byte & bitmask);
        }
        break;
    
    case 0xA000:
    case 0xB000:
        // Writing to External RAM
        if (external_ram_enable)
            ram.at(0x2000 * ram_bank_number + (address - 0xA000)) = byte;

        break;
    }
}

uint8_t Cartridge::mbc5_read(uint16_t address)
{
    switch (address & 0xF000)
    {
    case 0x0000:
    case 0x1000:
    case 0x2000:
    case 0x3000:
        return rom.at(address);

    case 0x4000:
    case 0x5000:
    case 0x6000:
    case 0x7000:
        return rom.at(0x4000 * rom_bank_number + (address - 0x4000));

    case 0xA000:
    case 0xB000:
        if (external_ram_enable)
            return ram.at(0x2000 * ram_bank_number + (address - 0xA000));
        else
            return 0xFF;
    }

    return 0x00;
}