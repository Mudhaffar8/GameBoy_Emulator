#pragma once

#include "cartridge.hpp"

#include <cstdint>

const size_t MEMORY_SIZE = 0x10000;

const uint16_t BOOT_ROM_START = 0x0000;
const uint16_t BOOT_ROM_END = 0x00FF;

/* Bank Zero - Static */
const uint16_t BANK_ZERO_START = 0x0000;
const uint16_t BANK_ZERO_END = 0x3FFF;

/* Bank N - Switchable ROM Bank */
const uint16_t BANK_N_START = 0x4000;
const uint16_t BANK_N_END = 0x7FFF;

/* Tile Maps */
const uint16_t TILE_MAP_START = 0x8000; 
const uint16_t TILE_MAP_END = 0x97FF;

const uint16_t BG_MAP_START = 0x9800; 
const uint16_t BG_MAP_END = 0x9FFF;

/* RAM */
/* Cartridge RAM */
const uint16_t CARTRIDGE_RAM_START = 0xA000; 
const uint16_t CARTRIDGE_RAM_END = 0xBFFF;


/* Work RAM */
const uint16_t WORK_RAM_START = 0xC000;
const uint16_t WORK_RAM_END = 0xDFFF;

const uint16_t ECHO_RAM_START = 0xE000;
const uint16_t ECHO_RAM_END = 0xFDFF;


/* Object Attribute Memory - Graphical sprites */
const uint16_t OAM_START = 0xFE00;
const uint16_t OAM_END = 0xFE9F;

/* Unusable Memory */
const uint16_t UNUSABLE_START = 0xFEA0;
const uint16_t UNUSABLE_END = 0xFEFF;

/* I/O Registers */
const uint16_t IO_REGISTERS_START = 0xFF00;
const uint16_t IO_REGISTERS_END = 0xFF7F;

// Bit 7 - Transfer Enable, Bit 1 - Clock Speed, Bit 0 - Clock Select
const uint16_t SERIAL_TRANSFER = 0xFF02;

/* Timer Registers*/
const uint16_t TIMA = 0xFF05;
const uint16_t TMA = 0xFF06;
const uint16_t TAC = 0xFF07;

const uint16_t INTERRUPT_FLAG = 0xFF0F; 

const uint16_t VIEWPORT_Y_POS = 0xFF42;
const uint16_t VIEWPORT_X_POS = 0xFF43;

const uint16_t BG_PALETTE = 0xFF47;

const uint16_t INTERRUPT_ENABLE = 0xFFFF; 


// 5.Select buttons	 4.Select d-pad  3.Start/Down  2.Select/Up  1.B/Left  0.A/Right
// if select d-pad set to 0, lower nibble contains directional inputs
// I/O  Registers
const uint16_t JOYPAD_INPUT = 0xFF00;

/* High RAM */
const uint16_t HIGH_RAM_START = 0xFF80;
const uint16_t HIGH_RAM_END = 0xFFFE;

class Memory
{
public:
    Memory();

    inline uint8_t read_byte(uint16_t address) const;
    inline uint8_t& read_byte_ref(uint16_t address);
    void write_byte(uint8_t byte, uint16_t address);
    
private:
    uint8_t memory[MEMORY_SIZE];

    bool load_cartridge(Cartridge& cartridge);
};

inline uint8_t Memory::read_byte(uint16_t address) const
{
    return memory[address];
}

inline uint8_t& Memory::read_byte_ref(uint16_t address)
{
    return memory[address];
}

inline void Memory::write_byte(uint8_t byte, uint16_t address)
{
    memory[address] = byte;
}
