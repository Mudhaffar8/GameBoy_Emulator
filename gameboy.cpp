#include "gameboy.hpp"

#include <fstream>
#include <iostream>
#include <chrono>

Gameboy::Gameboy(const std::string& rom_name) :
    cartridge(Cartridge::load_rom(rom_name)),
    mmu(cartridge.get()),
    cpu(mmu),
    ppu(mmu),
    joypad(mmu),
    timer(mmu),
    display(ppu)
{
    // if (!save_file.empty())
    //     read_save_file(save_file);
}

void Gameboy::run()
{
    uint32_t cycles_elapsed = 0;
    while (display.is_program_running())
    {
        auto start = std::chrono::steady_clock::now();

        display.handle_events();
                
        while (cycles_elapsed <= GBTiming::CYCLES_PER_FRAME)
        {
            const bool* state = SDL_GetKeyboardState(NULL);
            joypad.handle_inputs(state);
            
            uint32_t cycles = cpu.execute_instruction();
            timer.tick(cycles);
            if (cpu.is_double_speed())
            {            
                auto _ = cpu.execute_instruction();
                timer.tick(cycles);
            }

            ppu.tick(cycles);

            cycles_elapsed += cycles;
        }

        cycles_elapsed %= GBTiming::CYCLES_PER_FRAME;

        display.update_screen();

        auto end = std::chrono::steady_clock::now();
        double diff = std::chrono::duration<double, std::milli>(end - start).count();
        
        uint32_t time = (diff <= 16.67f) ? (16.67 - diff) : 0;
        SDL_Delay(time);
    }
}

void Gameboy::write_save_file()
{
    std::ofstream save_file("save_file.bin", std::ios::binary);

    save_file.write(reinterpret_cast<const char*>(cartridge->rom.data()), cartridge->rom.size());
    save_file.write(reinterpret_cast<const char*>(cartridge->ram.data()), cartridge->ram.size());
    save_file.write(reinterpret_cast<const char*>(cartridge->ram_bank_number), 2);
    save_file.write(reinterpret_cast<const char*>(cartridge->rom_bank_number), 1);
    save_file.write(reinterpret_cast<const char*>(cartridge->banking_mode), 1);
    save_file.write(reinterpret_cast<const char*>(cartridge->external_ram_enable), 1);
    save_file.write(reinterpret_cast<const char*>(cartridge->rtc_register_id), 1);
    save_file.write(reinterpret_cast<const char*>(cartridge->rtc_enable), 1);
    save_file.write(reinterpret_cast<const char*>(cartridge->is_rtc_mapped_to_ram), 1);

    save_file.write(reinterpret_cast<const char*>(mmu.rom_data.data()), mmu.rom_data.size());
    save_file.write(reinterpret_cast<const char*>(mmu.work_ram.data()), mmu.work_ram.size());
    save_file.write(reinterpret_cast<const char*>(mmu.high_ram.data()), mmu.high_ram.size());
    save_file.write(reinterpret_cast<const char*>(mmu.vram_bank_0.data()), mmu.vram_bank_0.size());
    save_file.write(reinterpret_cast<const char*>(mmu.vram_bank_1.data()), mmu.vram_bank_1.size());
    save_file.write(reinterpret_cast<const char*>(mmu.oam_data.data()), mmu.oam_data.size());
    save_file.write(reinterpret_cast<const char*>(mmu.bg_cram.data()), mmu.bg_cram.size());
    save_file.write(reinterpret_cast<const char*>(mmu.obj_cram.data()), mmu.obj_cram.size());
    save_file.write(reinterpret_cast<const char*>(mmu.io_registers.data()), mmu.io_registers.size());
    save_file.write(reinterpret_cast<const char*>(mmu.interrupt_enable), 1);

    save_file.write(reinterpret_cast<const char*>(cpu.AF.r16), 2);
    save_file.write(reinterpret_cast<const char*>(cpu.BC.r16), 2);
    save_file.write(reinterpret_cast<const char*>(cpu.DE.r16), 2);
    save_file.write(reinterpret_cast<const char*>(cpu.HL.r16), 2);
    save_file.write(reinterpret_cast<const char*>(cpu.PC), 2);
    save_file.write(reinterpret_cast<const char*>(cpu.SP), 2);
    save_file.write(reinterpret_cast<const char*>(cpu.IR), 1);
    save_file.write(reinterpret_cast<const char*>(cpu.IME), 1);
    save_file.write(reinterpret_cast<const char*>(cpu.is_halted), 1);
    save_file.write(reinterpret_cast<const char*>(cpu.double_speed_mode), 1);

    save_file.write(reinterpret_cast<const char*>(ppu.ppu_mode), 1);
    save_file.write(reinterpret_cast<const char*>(ppu.cycles_elapsed), 4);
    save_file.write(reinterpret_cast<const char*>(ppu.window_internal_scanline_y), 1);
    save_file.write(reinterpret_cast<const char*>(ppu.ppu_mode), 1);
}

void Gameboy::read_save_file(std::string& file_name)
{
    std::ifstream save_file(file_name, std::ios::binary | std::ios::in);
}