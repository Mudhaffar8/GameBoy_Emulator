#pragma once

#include <cstdint>

const int ROM_MAX_SIZE = 0x8000;
const int MBC1_MAX_SIZE = 0x10000;

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

const uint16_t NEW_LICENSE_CODE_START = 0x0144;
const uint16_t NEW_LICENSE_CODE_END = 0x0145;

const uint16_t CARTRIDGE_HEADER = 0x0147;

const uint16_t ROM_SIZE = 0x0148;

const uint16_t RAM_SIZE = 0x0149;

const uint16_t DESTINATION_CODE = 0x014A;

const uint16_t CARTRIDGE_TYPE = 0x0147;

// We're going to need classes for different mappers
// Make this Interface?
class Cartridge
{

private:
    bool load_rom(const char* path);
};