#pragma once 

#include "memory.hpp"
#include "interrupts.hpp"

#include <iostream>
#include <SDL3/SDL_keyboard.h>

// GBDev.gg8.se
// Bit 7 - Not used
// Bit 6 - Not used
// Bit 5 - P15 Select Button Keys      (0=Select)
// Bit 4 - P14 Select Direction Keys   (0=Select)
// Bit 3 - P13 Input Down  or Start    (0=Pressed) (Read Only)
// Bit 2 - P12 Input Up    or Select   (0=Pressed) (Read Only)
// Bit 1 - P11 Input Left  or Button B (0=Pressed) (Read Only)
// Bit 0 - P10 Input Right or Button A (0=Pressed) (Read Only)
namespace GBJoypad
{
    enum Buttons 
    {
        A = 0x01,
        B = 0x02,
        Select = 0x04,
        Start = 0x08
    };

    enum DPad
    {
        Right = 0x01,
        Left = 0x02,
        Up = 0x04,
        Down = 0x08
    };

    constexpr uint8_t SELECT_DPAD = 0x10;
    constexpr uint8_t SELECT_BUTTONS = 0x20;
};

namespace GBInput
{
    constexpr int DPAD_UP = SDL_SCANCODE_W;
    constexpr int DPAD_DOWN = SDL_SCANCODE_S;
    constexpr int DPAD_LEFT = SDL_SCANCODE_A;
    constexpr int DPAD_RIGHT = SDL_SCANCODE_D;
    constexpr int BUTTON_B = SDL_SCANCODE_J;
    constexpr int BUTTON_A = SDL_SCANCODE_K;
    constexpr int BUTTON_SELECT = SDL_SCANCODE_SPACE;
    constexpr int BUTTON_START = SDL_SCANCODE_RETURN;
}

class Joypad
{
public:
    Joypad(Mmu& _mmu);



    /* Input Handling */
    void handle_inputs(const bool* keyboard);
    
    void set_key(GBJoypad::Buttons dpad_bit);
    void set_key(GBJoypad::DPad button_bit);

    void unset_key(GBJoypad::Buttons button_bit);
    void unset_key(GBJoypad::DPad dpad_bit);

    void reset_input();

    inline bool is_buttons_selected() { return ((joypad_input & GBJoypad::SELECT_BUTTONS) == 0); }
    inline bool is_dpad_selected()  { return ((joypad_input & GBJoypad::SELECT_DPAD) == 0); }

    void print()
    {
        std::cout << "Buttons Selected: " << ((joypad_input & GBJoypad::SELECT_BUTTONS) != 0)
            << ", D-Pad Selected: " << ((joypad_input & GBJoypad::SELECT_DPAD) != 0)
            << ", Down/Start: " << ((joypad_input & GBJoypad::DPad::Down) != 0)
            << ", Up/Select: " << ((joypad_input & GBJoypad::DPad::Up) != 0)
            << ", Left/B: " << ((joypad_input & GBJoypad::DPad::Left) != 0)
            << ", Right/A: " << ((joypad_input & GBJoypad::DPad::Right) != 0)
            << '\n';
    }

private:
    Mmu& mmu;

    uint8_t& joypad_input;
};