#include "memory.hpp"

#include <fstream>
#include <filesystem>
#include <iostream>
#include <thread>

Mmu::Mmu(Cartridge* new_cartridge) : 
    cartridge(new_cartridge)
{
    
   std::copy(nintendo_logo.begin(), nintendo_logo.end(), rom_data.begin() + NINTENDO_LOGO_START);

    io_registers.at(OAM_DMA_TRANSFER - IO_REGISTERS_START) = 0xFF;
    io_registers.at(JOYPAD_INPUT - IO_REGISTERS_START) = 0x0F;

    io_registers.at(LCD_STATUS - IO_REGISTERS_START) = 0x80;
    io_registers.at(LCD_CONTROL - IO_REGISTERS_START) = 0x80;
}

void Mmu::load_cartridge(Cartridge* new_cartridge)
{
    cartridge = new_cartridge;
    std::copy(cartridge->rom.begin(), cartridge->rom.begin() + BANK_N_START, rom_data.begin());
}

bool Mmu::load_boot_rom(const std::string& path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);

    if (!file.is_open()) 
    {
        std::cerr << "File does not exist: " << path << std::endl;
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

void Mmu::oam_dma_transfer(uint8_t source)
{
    // Should I worry about DMA bus conflicts??
    uint16_t source_address = (source << 8);
    for (int i = 0; i < 0xA0; ++i)
    {
        uint8_t byte = read_byte(source_address + i);
        write_byte(byte, OAM_START + i);
    }
}

void Mmu::vram_dma_transfer(uint8_t vram_dma_len_mode)
{
    /// @todo Implement HDMA
    // https://jsgroth.dev/blog/posts/emulator-bugs-gbc-hdma/
    // This blog is super useful on how VRAM DMA Transfer works
    // and common emulation (& ROM) bugs

    // HBlank DMA = 0x10 bytes every HBlank
    std::cout << "VRAM Data Transfer!\n";
    bool is_hblank_dma = (vram_dma_len_mode & 0x80) != 0;
    int data_length = ((vram_dma_len_mode & 0x7F) + 1) * 0x10;
    // std::cout << "Data Length Before: " << data_length << '\n';

    // std::cout << "Data Length: " << std::hex << data_length << '\n';
    // std::cout << "HBlank DMA? " << is_hblank_dma << '\n';

    uint8_t vram_dma_source_high = io_registers.at(HDMA1 - IO_REGISTERS_START);
    uint8_t vram_dma_source_low = io_registers.at(HDMA2 - IO_REGISTERS_START);
    uint16_t source_address = ((vram_dma_source_high << 8) | vram_dma_source_low) & 0xFFF0;
    //std::cout << "Source Address: " << std::hex << +source_address << '\n';

    uint8_t vram_dma_dst_high = io_registers.at(HDMA3 - IO_REGISTERS_START);
    uint8_t vram_dma_dst_low = io_registers.at(HDMA4 - IO_REGISTERS_START);
    uint16_t dst_address = ((vram_dma_dst_high << 8) | vram_dma_dst_low) & 0x1FF0;
    dst_address = VRAM_START + dst_address;
    // std::cout << "Destination Address: " << std::hex << +dst_address << '\n';

    // std::cout << "---------------------\n";
 
    for (int i = 0; i < data_length; ++i)
    {
        uint8_t byte = read_byte(source_address + i);
        write_byte(byte, dst_address + i);
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
        cartridge->memory_write(byte, address);
        break;
    
    /* VRAM */
    /* Tile Data*/
    case 0x8000:
    case 0x9000:
        if ((read_io_reg(VRAM_BANK_SELECT) & 1) == 0)
            vram_bank_0.at(address - 0x8000) = byte;
        else 
            vram_bank_1.at(address - 0x8000) = byte;

        break;
    
    /* Cartridge RAM */
    case 0xA000:
    case 0xB000:
        cartridge->memory_write(byte, address);
        break;
    
    /* Work RAM Bank 0 */
    case 0xC000:
        work_ram.at(address - 0xC000) = byte;
        break;

    /* Work RAM Bank 1-7 */
    case 0xD000:
        {
            uint8_t wram_select = std::max((uint8_t)1, io_registers.at(WRAM_BANK_SELECT - IO_REGISTERS_START));
            int addr = (WORK_RAM_SIZE * wram_select) + (address - 0xD000);
            work_ram.at(addr) = byte;
        }
        break; 

    /* Echo RAM */
    case 0xE000:
        work_ram.at(address - 0xE000) = byte;
        break;
    
    case 0xF000:
        if (address <= ECHO_RAM_END)
            work_ram.at(address - 0xF000) = byte; 
        else if (address <= OAM_END)
            oam_data.at(address - OAM_START) = byte;
        else if (address <= UNUSABLE_END)
            break;
            //std::cout << "Illegal write to UNUSABLE @ " << std::hex << address << '\n';
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
            uint8_t& joypad_input = io_registers.at(JOYPAD_INPUT - IO_REGISTERS_START);
            joypad_input &= 0xF;
            joypad_input |= (byte & 0xF0);
        }
        break;
    
    case LCD_Y_COORDINATE:
        break;

    case KEY_1:
        {
            uint8_t& key_1 = io_registers.at(KEY_1 - IO_REGISTERS_START);
            key_1 &= 0x80;
            key_1 |= byte;
            break;
        }

    case BG_PALETTE_DATA:
        {
            uint8_t& bg_palette_idx = get_io_reg(BG_PALETTE_IDX);

            bool auto_increment = (bg_palette_idx & 0x80) != 0;
            uint8_t addr = bg_palette_idx & 0x3F;

            bg_cram.at(addr) = byte;

            if (auto_increment)
            {
                addr = (addr + 1) & 0x3F;
                bg_palette_idx &= 0x80;
                bg_palette_idx |= addr;
            }
        }
        break;

    case OBJ_PALETTE_DATA:
        {
            uint8_t& obj_palette_idx = get_io_reg(OBJ_PALETTE_IDX);

            bool auto_increment = (obj_palette_idx & 0x80) != 0;
            uint8_t addr = obj_palette_idx & 0x3F;

            obj_cram.at(addr) = byte;
            
            if (auto_increment)
            {
                addr = (addr + 1) & 0x3F;
                obj_palette_idx &= 0x80;
                obj_palette_idx |= addr;
            }
        }
        break;

    case HDMA5: // VRAM DMA Transfer
        vram_dma_transfer(byte);
        break;

    case WRAM_BANK_SELECT:
        io_registers.at(WRAM_BANK_SELECT - IO_REGISTERS_START) = byte & 0x7;
        break;

    case OAM_DMA_TRANSFER:
        io_registers.at(OAM_DMA_TRANSFER - IO_REGISTERS_START) = byte;
        oam_dma_transfer(byte);
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
        // if (cartridge) 
        return cartridge->memory_read(address);
        // else return rom_data.at(address);
    
    /* VRAM */
    case 0x8000:
    case 0x9000:
        if ((read_io_reg(VRAM_BANK_SELECT) & 1) == 0)
            return vram_bank_0.at(address - 0x8000);
        else 
            return vram_bank_1.at(address - 0x8000);
    
    /* Cartridge RAM */
    case 0xA000:
    case 0xB000:
        return cartridge->memory_read(address);
    
    /* Work RAM 0 */
    case 0xC000:
        return work_ram.at(address - 0xC000);

    /* Work RAM 0 */
    case 0xD000:
        {
            uint8_t wram_select = std::max((uint8_t)1, io_registers.at(WRAM_BANK_SELECT - IO_REGISTERS_START));
            int addr = (WORK_RAM_SIZE * wram_select) + (address - 0xD000);
            return work_ram.at(addr);
        }

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
            std::cout << "Illegal Read to UNUSABLE @ " <<  std::hex << address << '\n';
            return 0;
        }
        else if (address <= IO_REGISTERS_END)
            return read_io_reg(address);
        else if (address <= HIGH_RAM_END)
            return high_ram.at(address - HIGH_RAM_START);
        else 
            return interrupt_enable;
    
    default:
        std::cout << "Unknown memory Address @ " << std::hex << address << '\n';
        return 0;
    }
}

uint8_t Mmu::read_io_reg(int address) 
{ 
    switch(address)
    {
    case OBJ_PALETTE_DATA:
        {
            uint8_t obj_palette_idx = io_registers.at(OBJ_PALETTE_IDX - IO_REGISTERS_START);
            uint8_t addr = obj_palette_idx & 0x3F;

            return obj_cram.at(addr);
        }

    case BG_PALETTE_DATA:
        {
            uint8_t bg_palette_idx = io_registers.at(BG_PALETTE_IDX - IO_REGISTERS_START);
            uint8_t addr = bg_palette_idx & 0x3F;

            return bg_cram.at(addr);
        }

    case JOYPAD_INPUT:
        {
            uint8_t joypad_input = io_registers.at(JOYPAD_INPUT - IO_REGISTERS_START);
            if ((joypad_input & 0x30) == 0x30)
                return 0x3F;
            else 
                return joypad_input;
        }

    case KEY_1:
        return io_registers.at(KEY_1 - IO_REGISTERS_START);

    default:
        return io_registers.at(address - IO_REGISTERS_START);
    } 
}

uint8_t& Mmu::get_io_reg(int address) 
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

    std::copy(tileset_tiles.begin(), tileset_tiles.end(), vram_bank_0.begin());

    std::fill(vram_bank_0.begin(), vram_bank_0.begin() + TILE_9C00_OFFSET, 0x03);
    std::fill(vram_bank_0.begin() + TILE_9C00_OFFSET, vram_bank_0.end(), 0x01);

    vram_bank_0.at(8) = 0x04;
    vram_bank_0.at(64) = 0x00; // Make tile Mario's face

    vram_bank_0.at(44) = 0x00; // Make tile Mario's face
    vram_bank_0.at(8) = 0x02; // Make 8th tile checkered pattern
    vram_bank_0.at(4) = 0x05; // Make 4th tile checkered pattern
}