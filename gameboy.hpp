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

    void tick(); 

    /* Event Handling*/
    inline void handle_events() { display.handle_events(); }
    inline void handle_inputs() 
    { 
        const bool* state = SDL_GetKeyboardState(NULL);
        joypad.handle_inputs(state); 
    }

    /* Display */
    inline void update_display() { display.update_screen(); }

    /* */
    inline bool is_running() { return display.is_program_running(); }
    inline bool is_frame_completed() 
    { 
        if (cycles_elapsed < GBTiming::CYCLES_PER_FRAME)
            return false;
        
        cycles_elapsed %= GBTiming::CYCLES_PER_FRAME;
        return true; 
    }
private:
    Mmu mmu;
    Cpu cpu;
    Ppu ppu;
    Joypad joypad;
    Timer timer;
    Display display;

    uint32_t cycles_elapsed{};
};
