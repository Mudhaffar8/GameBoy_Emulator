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
constexpr uint16_t TOTAL_ROM_SIZE = BANK_N_END + 1;

/* RAM */
/* VRAM */
constexpr uint16_t VRAM_START = 0x8000; 
constexpr uint16_t VRAM_END = 0x9FFF; 
constexpr uint16_t VRAM_SIZE = (VRAM_END - VRAM_START) + 1; 

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
constexpr uint16_t WORK_RAM_0_START = 0xC000;
constexpr uint16_t WORK_RAM_0_END = 0xCFFF;

constexpr uint16_t WORK_RAM_X_START = 0xD000;
constexpr uint16_t WORK_RAM_X_END = 0xDFFF;
constexpr uint16_t WORK_RAM_SIZE = WORK_RAM_0_END - WORK_RAM_0_START + 1;

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

constexpr uint16_t OAM_DMA_TRANSFER = 0xFF46;

// Palatte Data
constexpr uint16_t BG_PALETTE = 0xFF47;
constexpr uint16_t OBJ_PALETTE_0 = 0xFF48;
constexpr uint16_t OBJ_PALETTE_1 = 0xFF49;

constexpr uint16_t WINDOW_Y_POS = 0xFF4A;
constexpr uint16_t WINDOW_X_POS = 0xFF4B;

// CPU Mode Select
// Only written to by CGB boot ROM
// DMG compatibility mode: 
// - 0 = Disabled (full CGB mode, for regular CGB cartridges), 
// - 1 = Enabled (for DMG only cartridges)
constexpr uint16_t KEY_0 = 0xFF4C;
// Bit 7: Current speed (Read-only): 0 = Normal-speed mode, 1 = Double-speed mode
// Bit 0: Switch armed (Read/Write): 0 = No, 1 = Armed
// In Double Speed Mode, following will be twice as fast:
// - The CPU
// - Timer and Divider Registers
// - Serial Port (Link Cable)
// - DMA Transfer to OAM
constexpr uint16_t KEY_1 = 0xFF4D;

constexpr uint16_t VRAM_BANK_SELECT = 0xFF4F;

constexpr uint16_t BOOT_ROM_MAPPING = 0xFF50;

/* CGB-only Registers */
/* VRAM DMA */
// VRAM DMA source (high, low) [write-only], 
// where to read data from
// Either in range 0000-7FF0 or A000-DFF0
constexpr uint16_t HDMA1 = 0xFF51; 
constexpr uint16_t HDMA2 = 0xFF52;
// VRAM DMA Destination (high, low) [write-only], 
// only reads bits 12-4, rest ignored and treated as 0
// Ranges within 8000-9FF0
constexpr uint16_t HDMA3 = 0xFF53; 
constexpr uint16_t HDMA4 = 0xFF54; 
// VRAM DMA Length/mod/start
// Used to intiate DMA transfer from ROM/RAM to VRAM
// Writing here starts transfer
// lower 7 bits specifiy transfer length (divided by 0x10, minus 1) 
constexpr uint16_t HDMA5 = 0xFF55; 

constexpr uint16_t IR_PORT = 0xFF56; // Ignoring this one ngl

/* CGB Palette Data */
// Bit 7: Auto-increment: 0 = Disabled; 1 = Enabled
// Bit 0-5: Address: Specifies which byte of BG Palette Memory can be accessed through BGPD
constexpr uint16_t BG_PALETTE_IDX = 0xFF68; 
// As each color is two bytes in size, you must read/write this register twice to access a whole color.
// This is made much easier through the use of the address 
// auto-increment: BGPI's "address" field is automatically 
// incremented (wrapping around from 63 back to 0) after 
// each write to this register, even if the write fails 
// due to CRAM being inaccessible. Reads, however, never 
// trigger auto-increment.
constexpr uint16_t BG_PALETTE_DATA = 0xFF69; 
// Work similarly to BGPI and BGD
constexpr uint16_t OBJ_PALETTE_IDX = 0xFF6A; 
constexpr uint16_t OBJ_PALETTE_DATA = 0xFF6B; 

// Which object priority mode to use
// Priority mode (Read/Write): 0 = CGB-style priority, 1 = DMG-style priority
constexpr uint16_t OBJ_PRIORITY_MODE = 0xFF6C; 
constexpr uint16_t WRAM_BANK_SELECT = 0xFF70; 

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

    void initialize_memory();

    /* Writing to memory */
    void write_byte(uint8_t byte, int address);
    void write_io_reg(uint8_t byte, int address);

    /* Reading from memory */
    uint8_t read_byte(int address);
    uint8_t read_io_reg(int address);
    uint8_t& get_io_reg(int address);

    /* Loading programs into memory */
    void load_cartridge(Cartridge* cartridge);
    bool load_boot_rom(const std::string& path);

    /* DMA Transfer */
    void oam_dma_transfer(uint8_t source);
    void vram_dma_transfer(uint8_t dma_mode_length);
    
    /* Testing */
    void load_test_tiles();

    inline uint8_t& get_interrupt_enable() { return interrupt_enable; }
    inline uint8_t& get_interrupt_flag() { return io_registers.at(INTERRUPT_FLAG - IO_REGISTERS_START); }

    /* CGB Helper Methods */
    bool is_cgb_mode() 
    { return rom_data.at(CGB_FLAG) == 0x80 || rom_data.at(CGB_FLAG) == 0xC0; }

    inline uint8_t read_vram_0(uint16_t address) { return vram_bank_0.at(address - VRAM_START); }
    inline uint8_t read_vram_1(uint16_t address) { return vram_bank_1.at(address - VRAM_START); }

    inline std::array<uint8_t, 64>& get_bg_cram() { return bg_cram; }
    inline std::array<uint8_t, 64>& get_obj_cram() { return obj_cram; }

    inline void print_bg_cram()
    {
        for (int i = 0; i < 7; ++i)
        {
            for (int j = 0; j < 8; ++j)
            {
                std::cout << "0x" << std::dec << +bg_cram[j + (8 * i)] << ", ";
            }
            std::cout << '\n';
        }
    }

    // #ff00ff
    // #ffff00
    // 
    inline void print_obj_cram()
    {
        for (int i = 0; i < 7; ++i)
        {
            for (int j = 0; j < 8; ++j)
            {
                std::cout << "0x" << std::dec << +obj_cram[j + (8 * i)] << ", ";
            }
            std::cout << '\n';
        }
    }

private:
    Cartridge* cartridge = nullptr;

    /* ROM code */
    std::array<uint8_t, TOTAL_ROM_SIZE> rom_data{};

    /* RAM */
    std::array<uint8_t, WORK_RAM_SIZE * 8> work_ram{};
    std::array<uint8_t, HIGH_RAM_SIZE> high_ram{};

    /* VRAM */
    std::array<uint8_t, VRAM_SIZE> vram_bank_0{};
    std::array<uint8_t, VRAM_SIZE> vram_bank_1{}; // CGB Only
    std::array<uint8_t, OAM_SIZE> oam_data{};
    // CGB-Only Colour RAM
    std::array<uint8_t, 64> bg_cram{};
    std::array<uint8_t, 64> obj_cram{}; // idx 0 at each palette is still transparent

    /* IO Registers */
    std::array<uint8_t, IO_REGISTERS_SIZE> io_registers{};

    uint8_t interrupt_enable{};
};