#include "joypad.hpp"

Joypad::Joypad(Mmu& _mmu) : 
    mmu(_mmu),
    joypad_input(mmu.get_io_reg(JOYPAD_INPUT))
{}

void Joypad::handle_inputs(const bool* state)
{
    if (is_dpad_selected())
    {
        set_key(GBJoypad::DPAD_UP, state[SDL_SCANCODE_W]);
        set_key(GBJoypad::DPAD_DOWN, state[SDL_SCANCODE_S]);
        set_key(GBJoypad::DPAD_LEFT, state[SDL_SCANCODE_A]);
        set_key(GBJoypad::DPAD_RIGHT, state[SDL_SCANCODE_D]);
    }
    else if (is_buttons_selected())
    {
        set_key(GBJoypad::BUTTON_B, state[SDL_SCANCODE_J]);
        set_key(GBJoypad::BUTTON_A, state[SDL_SCANCODE_K]);
        set_key(GBJoypad::BUTTON_SELECT, state[SDL_SCANCODE_SPACE]);
        set_key(GBJoypad::BUTTON_START, state[SDL_SCANCODE_RETURN]);
    }
}

void Joypad::reset_input()
{
    joypad_input |= (GBJoypad::DPAD_UP |
        GBJoypad::DPAD_DOWN |
        GBJoypad::DPAD_LEFT |
        GBJoypad::DPAD_RIGHT);
}

// NOTE: The lower nibble is Read-only. 
// Note that, rather unconventionally for the Game Boy,
// a button being pressed is seen as the corresponding bit 
// being 0, not 1.
void Joypad::set_key(uint8_t input_bit, bool cond)
{
    uint8_t prev_bit = joypad_input & input_bit;

    if (cond)
    {
        joypad_input &= ~input_bit;
        if (prev_bit != 0)
            GBInterrupts::request_interrupt(mmu, Interrupts::JoyPad);
    }
    else    
        joypad_input |= input_bit;
}