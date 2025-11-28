#pragma once

#include <cstdint>
#include <vector>

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

// Find ROM size with: 32 KiB × (1 << <value>)
const uint16_t ROM_SIZE = 0x0148; 
const uint16_t RAM_SIZE = 0x0149;

const uint16_t DESTINATION_CODE = 0x014A;
const uint16_t OLD_LICENSEE_CODE = 0x014B;

const uint16_t ROM_VERSION = 0x014C; // Normally 0x00
const uint16_t HEADER_CHECKSUM = 0x014D;

// Big-endian
const uint16_t GLOBAL_CHECKSUM_HIGH = 0x014E;
const uint16_t GLOBAL_CHECKSUM_LOW = 0x014F;

const uint16_t HEADER_SIZE = 0x0150;

// We're going to need classes for different mappers
// Make this Interface?
class Cartridge
{
public:
    Cartridge(size_t rom_size, size_t ram_size);
    
    enum CartridgeType 
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
        MBC3TimerRam_Battery = 0x10,
        MBC3 = 0x11
    };

    enum RamSize
    {
        NoRam = 0x00,
        Bank8K = 0x02,
        Bank32K = 0x03,
        Bank128K = 0x04,
        Bank64K = 0x05
    };
    
private:
    std::vector<uint8_t> rom;
    std::vector<uint8_t> ram;

    int curr_rom_bank{}, curr_ram_bank{};
};

bool read_header(const char* path);