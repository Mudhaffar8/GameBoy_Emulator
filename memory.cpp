#include "memory.hpp"

#include <fstream>
#include <filesystem>
#include <iostream>
#include <thread>

Mmu::Mmu()
{
    std::copy(nintendo_logo.begin(), nintendo_logo.end(), rom_data.begin() + NINTENDO_LOGO_START);

    io_registers.at(DMA - IO_REGISTERS_START) = 0xFF;
    io_registers.at(JOYPAD_INPUT - IO_REGISTERS_START) = 0x0F;
}

void Mmu::load_cartridge(Cartridge& cartridge)
{
    std::copy(cartridge.rom.begin(), cartridge.rom.end(), rom_data.begin());
}

bool Mmu::load_boot_rom(const std::string& path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);

    if (!file.is_open()) 
    {
        std::cerr << "File does not exist" << std::endl;
        return false;
    }

    size_t file_size = static_cast<size_t>(std::filesystem::file_size(path));
    if (file_size > TOTAL_ROM_SIZE)
    {
        std::cerr << "File size is too large" << std::endl;
        return false;
    }

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(rom_data.data()), rom_data.size());

    return true;
}

void Mmu::dma_transfer(uint8_t source)
{
    // Should I worry about DMA bus conflicts??
    uint16_t source_address = (source << 8);
    for (int i = 0; i < 0xA0; ++i)
    {
        uint8_t byte = read_byte(source_address + i);
        write_byte(byte, OAM_START + i);
    }
}

/* Writing from memory */
void Mmu::write_byte(uint8_t byte, int address)
{
    switch (address & 0xF000)
    {
    /* Bank Zero & Bank N */
    case 0x0000:
    case 0x1000:
    case 0x2000:
    case 0x3000:
    case 0x4000:
    case 0x5000:
    case 0x6000:
    case 0x7000:
        std::cout << "Illegal Write to ROM @ " << std::hex << address << '\n';
        break;
    
    /* VRAM */
    /* Tile Data*/
    case 0x8000:
        tile_data.at(address - 0x8000) = byte;
        break;
    
    /* Tile Data or Tile Maps */
    case 0x9000:
        if (address <= TILE_DATA_END)
            tile_data.at(address - 0x8000) = byte;
        else
            tile_map.at(address - TILE_MAP_START) = byte;
        
        break;
    
    /* Cartridge RAM */
    case 0xA000:
    case 0xB000:
        cartridge_ram.at(address - 0xA000) = byte;
        break;
    
    /* Work RAM */
    case 0xC000:
    case 0xD000:
        work_ram.at(address - 0xC000) = byte;
        break;
    
    /* Echo RAM */
    case 0xE000:
        work_ram.at(address - 0xE000) = byte;
        break;
    
    case 0xF000:
        if (address <= ECHO_RAM_END)
            work_ram.at(address - 0xE000) = byte;
        else if (address <= OAM_END)
            oam_data.at(address - OAM_START) = byte;
        else if (address <= UNUSABLE_END)
            std::cout << "Illegal write to UNUSABLE @ " << std::hex << address << '\n';
        else if (address <= IO_REGISTERS_END)
            write_io_reg(byte, address);
        else if (address <= HIGH_RAM_END)
            high_ram.at(address - HIGH_RAM_START) = byte;
        else 
            interrupt_enable = byte;
        
        break;
    }
}

void Mmu::write_io_reg(uint8_t byte, int address)
{

    switch(address)
    {
    // Lower nibble is read-only
    case JOYPAD_INPUT:
    {
        std::cout << "Writing Value: " << std::hex << +byte << '\n';
        //std::cout << "Initial Joypad Value: " << +io_registers.at(JOYPAD_INPUT - IO_REGISTERS_START) << '\n';
        uint8_t& joypad_input = io_registers.at(JOYPAD_INPUT - IO_REGISTERS_START);
        joypad_input &= 0xF;
        //std::cout << "Joypad After 0x0F: " << +io_registers.at(JOYPAD_INPUT - IO_REGISTERS_START) << '\n';
        joypad_input |= (byte & 0xF0);
        std::cout << "Final Joypad Value: " << std::hex << +io_registers.at(JOYPAD_INPUT - IO_REGISTERS_START) << '\n';
        //std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
        break;

    case DMA:
        io_registers.at(DMA - IO_REGISTERS_START) = byte;
        dma_transfer(byte);
        break;

    default:
        io_registers.at(address - IO_REGISTERS_START) = byte;
        break;
    }
}

/* Reading from memory */
uint8_t Mmu::read_byte(int address) 
{ 
    switch (address & 0xF000)
    {
    /* Bank Zero & Bank N */
    case 0x0000:
    case 0x1000:
    case 0x2000:
    case 0x3000:
    case 0x4000:
    case 0x5000:
    case 0x6000:
    case 0x7000:
        return rom_data.at(address);
    
    /* VRAM */
    /* Tile Data*/
    case 0x8000:
        return tile_data.at(address - 0x8000);
    
    /* Tile Data or Tile Maps */
    case 0x9000:
        if (address <= TILE_DATA_END)
            return tile_data.at(address - 0x8000);
            
        return tile_map.at(address - TILE_MAP_START);
    
    /* Cartridge RAM */
    case 0xA000:
    case 0xB000:
        return cartridge_ram.at(address - 0xA000);
    
    /* Work RAM */
    case 0xC000:
    case 0xD000:
        return work_ram.at(address - 0xC000);
    
    /* Echo RAM */
    case 0xE000:
        return work_ram.at(address - 0xE000);
    
    case 0xF000:
        if (address <= ECHO_RAM_END)
            return work_ram.at(address - 0xE000);
        else if (address <= OAM_END)
            return oam_data.at(address - OAM_START);
        else if (address <= UNUSABLE_END)
        {
            //std::cout << "Illegal Read to UNUSABLE @ " <<  std::hex << address << '\n';
            return 0;
        }
        else if (address <= IO_REGISTERS_END)
        {
            switch(address)
            {
                case JOYPAD_INPUT:
                    // if ((JOYPAD_INPUT & 0x30) == 0x30)
                    //     return 0x3F;
                    // else 
                        return io_registers.at(JOYPAD_INPUT - IO_REGISTERS_START);
                default:
                    return io_registers.at(address - IO_REGISTERS_START);
            } 
        }
        else if (address <= HIGH_RAM_END)
            return high_ram.at(address - HIGH_RAM_START);
        else 
            return interrupt_enable;
    
    default:
        std::cout << "Unknown memory Address @ " << std::hex << address << '\n';
        return 0;
    }
}

uint8_t& Mmu::read_io_reg(int address) 
{ 
    return io_registers.at(address - IO_REGISTERS_START);
}

void Mmu::load_test_tiles()
{
    std::vector<uint8_t> tileset_tiles 
    {
        0xFC,0xFC,0xFE,0xFE,0xDC,0x24,0xDC,0x74,0xDC,0x74,0x3E,0xC2,0x82,0xFE,0xFE,0xFE, // Mario's face
        0x00,0x00,0x3C,0x3C,0x7E,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x7E,0x00,0x3C, // Number zero
        0x55,0x33,0xAA,0xCC,0x55,0x33,0xAA,0xCC,0x55,0x33,0xAA,0xCC,0x55,0x33,0xAA,0xCC, // Checkered pattern
        0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F, // Other Checkered Pattern
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // Black Square
        0xFF,0x00,0x7E,0xFF,0x85,0x81,0x89,0x83,0x93,0x85,0xA5,0x8B,0xC9,0x97,0x7E,0xFF, // Window Pokemon 
        0x00,0x00,0x18,0x1C,0x3C,0x26,0x62,0x43,0x7E,0x7F,0x62,0x43,0x62,0x43,0x00,0x00, // Letter "A"
        0x00,0x00,0x3E,0x3C,0x43,0x62,0x40,0x60,0x40,0x60,0x43,0x62,0x3E,0x3C,0x00,0x00, // Letter "C"
    };

    constexpr uint16_t TILE_9C00_OFFSET = TILE_MAP_9C00_START - TILE_MAP_START;

    std::copy(tileset_tiles.begin(), tileset_tiles.end(), tile_data.begin());

    std::fill(tile_map.begin(), tile_map.begin() + TILE_9C00_OFFSET, 0x03);
    std::fill(tile_map.begin() + TILE_9C00_OFFSET, tile_map.end(), 0x01);

    tile_map.at(8) = 0x04;
    tile_map.at(64) = 0x00; // Make tile Mario's face

    tile_map.at(44) = 0x00; // Make tile Mario's face
    tile_map.at(8) = 0x02; // Make 8th tile checkered pattern
    tile_map.at(4) = 0x05; // Make 4th tile checkered pattern
}