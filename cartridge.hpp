#pragma once

#include <cstdint>
#include <vector>
#include <iostream>
#include <memory>
#include <array>
#include <variant>

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

// 0x80 = CGB but backwards Compatible w/ gameboys
// 0xC0 = Works on CGB Mode only
// Otherwise, DMG Mode only
const uint16_t CGB_FLAG = 0x0143;

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

    static std::unique_ptr<Cartridge> load_rom(const std::string path);

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
        MBC3Ram = 0x12,
        MBC3RamBattery = 0x13,
        MBC5 = 0x19,
        MBC5Ram = 0x1A,
        MBC5RamBattery = 0x1B
    };

    enum RamSize : uint8_t
    {
        NoRam = 0x00,
        Bank8K = 0x02,
        Bank32K = 0x03,
        Bank128K = 0x04,
        Bank64K = 0x05
    };

    std::vector<uint8_t> rom;
    std::vector<uint8_t> ram;

    uint8_t memory_read(uint16_t address);
    uint8_t mbc1_read(uint16_t address);
    uint8_t mbc3_read(uint16_t address);
    uint8_t mbc5_read(uint16_t address);

    void memory_write(uint8_t byte, uint16_t address);
    void mbc1_write(uint8_t byte, uint16_t address);
    void mbc3_write(uint8_t byte, uint16_t address);
    void mbc5_write(uint8_t byte, uint16_t address);

    /* Debugging */
    void print();    

private:
    /* MBC1 Registers */
    uint16_t rom_bank_number = 1; // Making this 16-bit may cause problems
    uint8_t ram_bank_number{}; // MBC1: unsigned 2-bit number, MBC5: 4-bit
 
    bool banking_mode = false;
    bool external_ram_enable = false;

    /* MBC3 Registers */
    uint8_t rtc_register_id = 0;

    bool rtc_enable = false;
    bool is_rtc_mapped_to_ram = false;

    struct RtcRegister
    {
        uint8_t reg8{};
        uint8_t latch{};

        void set_latch(uint8_t byte) { latch = byte; }
        void set_both(uint8_t byte) { latch = byte, reg8 = byte; }
    };

    RtcRegister rtc_s; // 6-bit for counting seconds
    RtcRegister rtc_m; // 6-bit for counting minutes
    RtcRegister rtc_h; // 5-bit for counting hours
    RtcRegister rtc_dl; // 8-bit for repr. lower 8-bits of total 9-bit day counter
    RtcRegister rtc_dh; 

    const uint8_t RTC_S_BITMASK = 0b00111111;
    const uint8_t RTC_M_BITMASK = 0b00111111;
    const uint8_t RTC_H_BITMASK = 0b00011111;
    const uint8_t RTC_DH_BITMASK = 0b11000001;


    // Use function pointers depending on MBC Type?
    // int (*memory_read)(int);
    // int (*memory_write)(int, int);

    /* Helper Methods */
    static size_t get_ram_size(uint8_t ram_size);
    static size_t get_rom_size(uint8_t rom_size);

    inline uint8_t get_size_kb(size_t size) { return size / ONE_KB; }
    inline uint8_t get_num_rom_banks() { return rom.size() / (ONE_KB * 16); }
    inline uint8_t get_num_ram_banks() { return ram.size() / (ONE_KB * 8); }

    /* MBC3 Helper Methods */
    void write_to_rtc_register(uint8_t byte);
    uint8_t read_to_rtc_register();

    friend class Gameboy;
};