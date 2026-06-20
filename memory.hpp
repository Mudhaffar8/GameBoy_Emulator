#pragma once

#include "cartridge.hpp"

#include <cstdint>
#include <array>
#include <iostream>


/// @todo Add namespaces
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
constexpr uint16_t BANK_SIZE = BANK_ZERO_END + 1;

/* Bank N - Switchable ROM Bank */
constexpr uint16_t BANK_N_START = 0x4000;
constexpr uint16_t BANK_N_END = 0x7FFF;
constexpr uint16_t BANKS_SIZE = BANK_N_END + 1;

/* RAM */
/* VRAM */
constexpr uint16_t TILE_DATA_ADDR0_START = 0x8000; 
constexpr uint16_t TILE_DATA_ADDR1_START = 0x9000;
constexpr uint16_t TILE_DATA_END = 0x97FF;
constexpr uint16_t TILE_DATA_SIZE = TILE_DATA_END - TILE_DATA_ADDR0_START + 1;

/* Tile Map Data */
constexpr uint16_t TILE_MAP_START = 0x9800; 
constexpr uint16_t TILE_MAP_END = 0x9FFF;
constexpr uint16_t TILE_MAP_SIZE = TILE_MAP_END - TILE_MAP_START + 1;

constexpr uint16_t TILE_MAP_9800_START = 0x9800; 
constexpr uint16_t TILE_MAP_9800_END = 0x9BFF;

constexpr uint16_t TILE_MAP_9C00_START = 0x9C00; 
constexpr uint16_t TILE_MAP_9C00_END = 0x9FFF;

/* Switchable Cartridge RAM */
constexpr uint16_t CARTRIDGE_RAM_START = 0xA000; 
constexpr uint16_t CARTRIDGE_RAM_END = 0xBFFF;
constexpr uint16_t CARTRIDGE_RAM_SIZE = CARTRIDGE_RAM_END - CARTRIDGE_RAM_START + 1;

/* Work RAM */
constexpr uint16_t WORK_RAM_START = 0xC000;
constexpr uint16_t WORK_RAM_END = 0xDFFF;
constexpr uint16_t WORK_RAM_SIZE = WORK_RAM_END - WORK_RAM_START + 1;

constexpr uint16_t ECHO_RAM_START = 0xE000;
constexpr uint16_t ECHO_RAM_END = 0xFDFF;

/* Object Attribute Memory - Graphical sprites */
constexpr uint16_t OAM_START = 0xFE00;
constexpr uint16_t OAM_END = 0xFE9F;
constexpr uint16_t OAM_SIZE = OAM_END - OAM_START + 1;

/* Unusable Memory */
constexpr uint16_t UNUSABLE_START = 0xFEA0;
constexpr uint16_t UNUSABLE_END = 0xFEFF;

/* I/O Registers */
constexpr uint16_t IO_REGISTERS_START = 0xFF00;
constexpr uint16_t IO_REGISTERS_END = 0xFF7F;
constexpr uint16_t IO_REGISTERS_SIZE = IO_REGISTERS_END - IO_REGISTERS_START + 1;

// 5.Select buttons	 4.Select d-pad  3.Start/Down  2.Select/Up  1.B/Left  0.A/Right
// if select d-pad set to 0, lower nibble contains directional inputs
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

// Sound Registers would go Here

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
constexpr uint16_t HIGH_RAM_SIZE = HIGH_RAM_END - HIGH_RAM_START + 1;

/// @brief Handles reads and writes to the Game Boy's addressable memory.
/// Provides functions for accessing memory and loading cartridge/ROM data into memory.
class Mmu
{
public:
    Mmu();

    /* Writing to memory */
    void write_byte(uint8_t byte, int address);
    void write_io_reg(uint8_t byte, int address);

    /* Reading from memory */
    uint8_t read_byte(int address);
    uint8_t& read_io_reg(int address);

    /* Loading programs into memory */
    bool load_cartridge(Cartridge& cartridge);
    bool load_rom(const char* path); /// @note For Testing Purposes

    void dma_transfer(uint8_t source);

    inline uint8_t get_interrupt_enable() { return interrupt_enable; }
    inline uint8_t get_interrupt_flag() { return io_registers.at(INTERRUPT_FLAG - IO_REGISTERS_START); }
private:
    Cartridge* cartridge;

    /* ROM code */
    std::array<uint8_t, BANKS_SIZE> rom_data{};

    /* RAM */
    std::array<uint8_t, CARTRIDGE_RAM_SIZE> cartridge_ram{};
    std::array<uint8_t, WORK_RAM_SIZE> work_ram{};
    std::array<uint8_t, HIGH_RAM_SIZE> high_ram{};

    /* VRAM */
    std::array<uint8_t, TILE_MAP_SIZE> tile_map{};
    std::array<uint8_t, TILE_DATA_SIZE> tile_data{};
    std::array<uint8_t, OAM_SIZE> oam_data{};

    /* IO Registers */
    std::array<uint8_t, IO_REGISTERS_SIZE> io_registers{};

    uint8_t interrupt_enable{};
};