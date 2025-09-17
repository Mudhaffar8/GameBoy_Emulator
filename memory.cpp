#pragma once

#include "memory.hpp"

#include <fstream>
#include <iostream>

Memory::Memory()
{}

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

bool Memory::load_cartridge(Cartridge& cartridge)
{

}