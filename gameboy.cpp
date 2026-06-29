#include "gameboy.hpp"

#include <fstream>
#include <iostream>
#include <chrono>

Gameboy::Gameboy(const std::string& rom_name, std::string save_file) :
    cartridge(Cartridge::load_rom(rom_name)),
    mmu(cartridge.get()),
    cpu(mmu),
    ppu(mmu),
    joypad(mmu),
    timer(mmu),
    display(ppu, settings)
{
    if (!save_file.empty())
        read_save_file(save_file);
}

void Gameboy::run()
{
    uint32_t cycles_elapsed = 0;
    while (display.is_program_running())
    {
        auto start = std::chrono::steady_clock::now();

        display.handle_events();

        if (settings.save_stage_trigger)
        {
            settings.save_stage_trigger = false;
            write_save_file();
        }
                
        while (!ppu.trigger_redisplay)
        {
            const bool* state = SDL_GetKeyboardState(NULL);
            joypad.handle_inputs(state);
            
            uint32_t cycles = cpu.execute_instruction();
            timer.tick(cycles);

            ppu.tick(cycles);

            cycles_elapsed += cycles;
        }

        ppu.trigger_redisplay = false;

        cycles_elapsed %= GBTiming::CYCLES_PER_FRAME;

        display.update_screen();

        auto end = std::chrono::steady_clock::now();
        double diff = std::chrono::duration<double, std::milli>(end - start).count();
        
        //std::cout << "Frame Drawn!\n";
        /// @todo change wait time depending on cycles past since last frame update
        uint32_t time = (diff <= 16.67f) ? (16.67 - diff) : 0;
        SDL_Delay(time);
    }
}

void Gameboy::write_save_file()
{
    std::string name{};
    name.reserve(TITLE_END - TITLE_END);

    for (int i = TITLE_START; i <= TITLE_END; ++i)
        name.push_back(cartridge->rom.at(i));

    std::ofstream save_file("./save_files/" + name + ".bin", std::ios::binary);
    if (!save_file.is_open())
    {
        std::cerr << "Could not write to file!\n";
        return;
    }

    save_file.write(reinterpret_cast<const char*>(cartridge->rom.data()), HEADER_SIZE);
    save_file.write(reinterpret_cast<const char*>(cartridge->ram.data()), cartridge->ram.size());
    save_file.write(reinterpret_cast<const char*>(&cartridge->ram_bank_number), 1);
    save_file.write(reinterpret_cast<const char*>(&cartridge->rom_bank_number), 2);
    save_file.write(reinterpret_cast<const char*>(&cartridge->banking_mode), 1);
    save_file.write(reinterpret_cast<const char*>(&cartridge->external_ram_enable), 1);
    save_file.write(reinterpret_cast<const char*>(&cartridge->rtc_register_id), 1);
    save_file.write(reinterpret_cast<const char*>(&cartridge->rtc_enable), 1);
    save_file.write(reinterpret_cast<const char*>(&cartridge->is_rtc_mapped_to_ram), 1);

    save_file.write(reinterpret_cast<const char*>(mmu.work_ram.data()), mmu.work_ram.size());
    save_file.write(reinterpret_cast<const char*>(mmu.high_ram.data()), mmu.high_ram.size());
    save_file.write(reinterpret_cast<const char*>(mmu.vram.data()), mmu.vram.size());
    save_file.write(reinterpret_cast<const char*>(mmu.oam_data.data()), mmu.oam_data.size());
    save_file.write(reinterpret_cast<const char*>(mmu.io_registers.data()), mmu.io_registers.size());
    save_file.write(reinterpret_cast<const char*>(&mmu.interrupt_enable), 1);

    save_file.write(reinterpret_cast<const char*>(&cpu.AF.r16), 2);
    save_file.write(reinterpret_cast<const char*>(&cpu.BC.r16), 2);
    save_file.write(reinterpret_cast<const char*>(&cpu.DE.r16), 2);
    save_file.write(reinterpret_cast<const char*>(&cpu.HL.r16), 2);
    save_file.write(reinterpret_cast<const char*>(&cpu.PC), 2);
    save_file.write(reinterpret_cast<const char*>(&cpu.SP), 2);
    save_file.write(reinterpret_cast<const char*>(&cpu.IR), 1);
    save_file.write(reinterpret_cast<const char*>(&cpu.IME), 1);
    save_file.write(reinterpret_cast<const char*>(&cpu.is_halted), 1);

    save_file.close();

    std::cout << "Successfully Saved File!\n";
}

void Gameboy::read_save_file(std::string& file_name)
{
    std::ifstream save_file("./save_files/" + file_name, std::ios::binary | std::ios::in);

    if (!save_file.is_open())
    {
        std::cerr << "Could not read from file!\n";
        return;
    }

    for (int i = TITLE_START; i <= TITLE_END; ++i)
    {
        save_file.seekg(i);
        if (save_file.get() != cartridge->rom.at(i))
        {
            std::cerr << "\nWrong Game!\n";
            return;
        }
    }

    save_file.seekg(0);

    save_file.read(reinterpret_cast<char*>(cartridge->rom.data()), HEADER_SIZE);
    save_file.read(reinterpret_cast<char*>(cartridge->ram.data()), cartridge->ram.size());
    save_file.read(reinterpret_cast<char*>(&cartridge->ram_bank_number), 1);
    save_file.read(reinterpret_cast<char*>(&cartridge->rom_bank_number), 2);
    save_file.read(reinterpret_cast<char*>(&cartridge->banking_mode), 1);
    save_file.read(reinterpret_cast<char*>(&cartridge->external_ram_enable), 1);
    save_file.read(reinterpret_cast<char*>(&cartridge->rtc_register_id), 1);
    save_file.read(reinterpret_cast<char*>(&cartridge->rtc_enable), 1);
    save_file.read(reinterpret_cast<char*>(&cartridge->is_rtc_mapped_to_ram), 1);

    save_file.read(reinterpret_cast<char*>(mmu.work_ram.data()), mmu.work_ram.size());
    save_file.read(reinterpret_cast<char*>(mmu.high_ram.data()), mmu.high_ram.size());
    save_file.read(reinterpret_cast<char*>(mmu.vram.data()), mmu.vram.size());
    save_file.read(reinterpret_cast<char*>(mmu.oam_data.data()), mmu.oam_data.size());
    save_file.read(reinterpret_cast<char*>(mmu.io_registers.data()), mmu.io_registers.size());
    save_file.read(reinterpret_cast<char*>(&mmu.interrupt_enable), 1);

    save_file.read(reinterpret_cast<char*>(&cpu.AF.r16), 2);
    save_file.read(reinterpret_cast<char*>(&cpu.BC.r16), 2);
    save_file.read(reinterpret_cast<char*>(&cpu.DE.r16), 2);
    save_file.read(reinterpret_cast<char*>(&cpu.HL.r16), 2);
    save_file.read(reinterpret_cast<char*>(&cpu.PC), 2);
    save_file.read(reinterpret_cast<char*>(&cpu.SP), 2);
    save_file.read(reinterpret_cast<char*>(&cpu.IR), 1);
    save_file.read(reinterpret_cast<char*>(&cpu.IME), 1);
    save_file.read(reinterpret_cast<char*>(&cpu.is_halted), 1);

    save_file.close();

    std::cout << "Successfully Read Save File!\n";
}