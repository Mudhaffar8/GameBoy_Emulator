#pragma once

#include <cstdint>
#include <vector>
#include <iostream>
#include <memory>
#include <array>

// TODO: Add namespaces

/* Cartridge Header */
const uint16_t ENTRY_POINT_START = 0x0100;
const uint16_t ENTRY_POINT_END = 0x0103;

constexpr uint16_t NINTENDO_LOGO_START = 0x0104;
constexpr uint16_t NINTENDO_LOGO_END = 0x0133;
constexpr uint16_t NINTENDO_LOGO_LEN = NINTENDO_LOGO_END - NINTENDO_LOGO_START + 1;

const uint16_t TITLE_START = 0x0134; 
const uint16_t TITLE_END = 0x0143;

const uint16_t MANUFACTURER_CODE_START = 0x013F;
const uint16_t MANUFACTURER_CODE_END = 0x0142;

const uint16_t NEW_LICENSEE_CODE_START = 0x0144;
const uint16_t NEW_LICENSEE_CODE_END = 0x0145;

const uint16_t SGB_FLAG = 0x0146;

const uint16_t CARTRIDGE_TYPE = 0x0147;

const uint16_t ROM_SIZE = 0x0148; // Find ROM size with: 32 KiB × (1 << <value>)
const uint16_t RAM_SIZE = 0x0149;

const uint16_t DESTINATION_CODE = 0x014A;
const uint16_t OLD_LICENSEE_CODE = 0x014B;

const uint16_t ROM_VERSION = 0x014C; // Normally 0x00
const uint16_t HEADER_CHECKSUM = 0x014D;

// Big-endian
const uint16_t GLOBAL_CHECKSUM_HIGH = 0x014E;
const uint16_t GLOBAL_CHECKSUM_LOW = 0x014F;

constexpr uint16_t HEADER_SIZE = 0x0150;

constexpr static std::array<uint8_t, NINTENDO_LOGO_LEN> nintendo_logo = {
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 
    0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,  
    0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 
    0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,  
    0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 
    0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
};

constexpr int ONE_KB = 1024;

/// @brief Represents Game Boy cartridge with ROM data and optional RAM.
/// Also supports MBC1.
class Cartridge
{
public:
    Cartridge(std::vector<uint8_t>& rom_data, std::vector<uint8_t>& ram_data);

    enum Type : uint8_t
    {
        RomOnly = 0x00,
        MBC1 = 0x01,
        MBC1Ram = 0x02,
        MBC1RamBattery = 0x03,
        MBC2 = 0x05,
        MBC2Battery = 0x06,
        RomRam = 0x08,
        RomRamBattery = 0x09,
        MBC3TimerBattery = 0x0F,
        MBC3TimerRamBattery = 0x10,
        MBC3 = 0x11,
        MBC5 = 0x19,
        MBC5Ram = 0x1A
    };

    enum RamSize : uint8_t
    {
        NoRam = 0x00,
        Bank8K = 0x02,
        Bank32K = 0x03,
        Bank128K = 0x04,
        Bank64K = 0x05
    };

    const std::vector<uint8_t> rom;
    std::vector<uint8_t> ram;

    static std::unique_ptr<Cartridge> load_rom(const std::string path);

    uint8_t memory_read(uint16_t address);
    void memory_write(uint8_t byte, uint16_t address);

    void mbc1_write(uint8_t byte, uint16_t address);
    uint8_t mbc1_read(uint16_t address);

    /* Debugging */
    void print();    
    
private:
    /* MBC1 Registers */
    uint8_t rom_bank_number{}; // unsigned 5-bit number
    uint8_t ram_bank_number{}; // unsigned 2-bit number
 
    bool banking_mode = false;
    bool external_ram_enable = false;

    /* Helper Methods */
    static size_t get_ram_size(uint8_t ram_size);
    static size_t get_rom_size(uint8_t rom_size);

    inline uint8_t get_size_kb(size_t size) { return size / ONE_KB; }
    inline uint8_t get_num_rom_banks() { return rom.size() / (ONE_KB * 16); }
    inline uint8_t get_num_ram_banks() { return ram.size() / (ONE_KB * 8); }
};