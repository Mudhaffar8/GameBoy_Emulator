#pragma once

#include "cartridge.hpp"
#include "memory.hpp"
#include "cpu.hpp"
#include "ppu.hpp"
#include "joypad.hpp"
#include "display.hpp"
#include "timer.hpp"

class Gameboy
{
public:
    Gameboy(const std::string& rom_name);

    void run();

private:
    std::unique_ptr<Cartridge> cartridge;
    Mmu mmu;
    Cpu cpu;
    Ppu ppu;
    Joypad joypad;
    Timer timer;
    Display display;

    /* Save File Handling */
    void write_save_file();
    void read_save_file(std::string& save_file);
};
