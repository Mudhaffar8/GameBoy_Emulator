#pragma once

#include "cartridge.hpp"

#include <cstdint>

// TODO: Add namespaces
constexpr size_t MEMORY_SIZE = 0x10000;

constexpr uint16_t BOOT_ROM_START = 0x0000;
constexpr uint16_t BOOT_ROM_END = 0x00FF;

/* Interrupt Addresses */
constexpr uint16_t VBLANK_INTERRUPT_START = 0x0040;
constexpr uint16_t STAT_INTERRUPT_START = 0x0048;
constexpr uint16_t TIMER_INTERRUPT_STARRT = 0x0050;
constexpr uint16_t SERIAL_INTERRUPT_START = 0x0058;
constexpr uint16_t JOYPAD_INTERRUPT_START = 0x0060;

/* Bank Zero - Static */
constexpr uint16_t BANK_ZERO_START = 0x0000;
constexpr uint16_t BANK_ZERO_END = 0x3FFF;

constexpr uint16_t ROM_CODE_START = 0x0150; 

/* Bank N - Switchable ROM Bank */
constexpr uint16_t BANK_N_START = 0x4000;
constexpr uint16_t BANK_N_END = 0x7FFF;

/* RAM */
/* VRAM */
constexpr uint16_t TILE_MAP_START = 0x8000; 
constexpr uint16_t TILE_MAP_END = 0x97FF;

constexpr uint16_t BG_MAP_START = 0x9800; 
constexpr uint16_t BG_MAP_END = 0x9FFF;


/* Cartridge RAM */
constexpr uint16_t CARTRIDGE_RAM_START = 0xA000; 
constexpr uint16_t CARTRIDGE_RAM_END = 0xBFFF;


/* Work RAM */
constexpr uint16_t WORK_RAM_START = 0xC000;
constexpr uint16_t WORK_RAM_END = 0xDFFF;

constexpr uint16_t ECHO_RAM_START = 0xE000;
constexpr uint16_t ECHO_RAM_END = 0xFDFF;


/* Object Attribute Memory - Graphical sprites */
constexpr uint16_t OAM_START = 0xFE00;
constexpr uint16_t OAM_END = 0xFE9F;

/* Unusable Memory */
constexpr uint16_t UNUSABLE_START = 0xFEA0;
constexpr uint16_t UNUSABLE_END = 0xFEFF;

/* I/O Registers */
constexpr uint16_t IO_REGISTERS_START = 0xFF00;
constexpr uint16_t IO_REGISTERS_END = 0xFF7F;

// 5.Select buttons	 4.Select d-pad  3.Start/Down  2.Select/Up  1.B/Left  0.A/Right
// if select d-pad set to 0, lower nibble contains directional inputs
// I/O  Registers
constexpr uint16_t JOYPAD_INPUT = 0xFF00;

// Serial Transfers
// Bit 7 - Transfer Enable, Bit 1 - Clock Speed, Bit 0 - Clock Select
constexpr uint16_t SERIAL_TRANSFER_DATA = 0xFF01;
constexpr uint16_t SERIAL_TRANSFER_CONTROL = 0xFF02;

/* Timer Registers*/
constexpr uint16_t DIVIDER_REGISTER = 0xFF04; // DIV

constexpr uint16_t TIMER_COUNTER = 0xFF05; // TIMA
constexpr uint16_t TIMER_MODULO = 0xFF06; // TMA
constexpr uint16_t TIMER_CONTROL = 0xFF07; // TAC

constexpr uint16_t INTERRUPT_FLAG = 0xFF0F; 

// Sound Register would go Here

// PPU Hardware Registers
constexpr uint16_t LCD_CONTROL = 0xFF40; 
constexpr uint16_t LCD_STATUS = 0xFF41; 

constexpr uint16_t VIEWPORT_Y_POS = 0xFF42;
constexpr uint16_t VIEWPORT_X_POS = 0xFF43;

constexpr uint16_t LCD_Y_COORDINATE = 0xFF44;
constexpr uint16_t LY_COMPARE = 0xFF45;

constexpr uint16_t DMA = 0xFF46;

// Palatte Data
constexpr uint16_t BG_PALETTE = 0xFF47;
constexpr uint16_t OBJ_PALETTE_0 = 0xFF48;
constexpr uint16_t OBJ_PALETTE_1 = 0xFF49;

constexpr uint16_t WINDOW_Y_POS = 0xFF4A;
constexpr uint16_t WINDOW_X_POS = 0xFF4B;

constexpr uint16_t BOOT_ROM_MAPPING = 0xFF50;

constexpr uint16_t INTERRUPT_ENABLE = 0xFFFF; 

/* High RAM */
constexpr uint16_t HIGH_RAM_START = 0xFF80;
constexpr uint16_t HIGH_RAM_END = 0xFFFE;

class Memory
{
public:
    Memory();

    inline uint8_t read_byte(uint16_t address) const;
    inline uint8_t& read_byte_ref(uint16_t address);
    inline void write_byte(uint8_t byte, uint16_t address);

    bool load_cartridge(Cartridge& cartridge);
    bool load_rom(const char* path); // For Testing Purposes
    
private:
    uint8_t memory[MEMORY_SIZE];
};

// TODO: Perform checks to prevent illegal read/writes to memory
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
