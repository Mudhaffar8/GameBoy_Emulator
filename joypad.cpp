#include "joypad.hpp"

Joypad::Joypad(Mmu& _mmu) : 
    mmu(_mmu),
    joypad_input(mmu.read_io_reg(JOYPAD_INPUT))
{}

void Joypad::handle_inputs(const bool* state)
{
    if (!is_buttons_selected() && !is_dpad_selected())
        return; 

    if (state[SDL_SCANCODE_W])
        set_key(GBJoypad::DPad::Up);
    else
        unset_key(GBJoypad::DPad::Up);

    if (state[SDL_SCANCODE_S]) 
        set_key(GBJoypad::DPad::Down);
    else 
        unset_key(GBJoypad::DPad::Down);

    if (state[SDL_SCANCODE_A])
        set_key(GBJoypad::DPad::Left);
    else 
        unset_key(GBJoypad::DPad::Left);

    if (state[SDL_SCANCODE_D])
        set_key(GBJoypad::DPad::Right);
    else
        unset_key(GBJoypad::DPad::Right);

    if (state[SDL_SCANCODE_J])
        set_key(GBJoypad::Buttons::B);
    else
        unset_key(GBJoypad::Buttons::B);

    if (state[SDL_SCANCODE_K])
        set_key(GBJoypad::Buttons::A);
    else
        unset_key(GBJoypad::Buttons::A);
    
    if (state[SDL_SCANCODE_SPACE])
        set_key(GBJoypad::Buttons::Select);
    else
        unset_key(GBJoypad::Buttons::Select);

    if (state[SDL_SCANCODE_RETURN])
        set_key(GBJoypad::Buttons::Start);
    else
        unset_key(GBJoypad::Buttons::Start);
}

void Joypad::reset_input()
{
    joypad_input |= (GBJoypad::DPad::Up |
        GBJoypad::DPad::Left |
        GBJoypad::DPad::Down |
        GBJoypad::DPad::Right);
}

// NOTE: The lower nibble is Read-only. 
// Note that, rather unconventionally for the Game Boy,
// a button being pressed is seen as the corresponding bit 
// being 0, not 1.
void Joypad::set_key(GBJoypad::DPad dpad_bit)
{
    uint8_t prev_bit = joypad_input & dpad_bit;
    if (is_dpad_selected())
    {
        joypad_input &= ~dpad_bit;

        if (prev_bit != 0)
            GBInterrupts::request_interrupt(mmu, Interrupts::JoyPad);
    }
}

void Joypad::set_key(GBJoypad::Buttons button_bit)
{
    uint8_t prev_bit = joypad_input & button_bit;
    if (is_buttons_selected())
    {
        joypad_input &= ~button_bit;

        if (prev_bit != 0)
            GBInterrupts::request_interrupt(mmu, Interrupts::JoyPad);
    }
}

void Joypad::unset_key(GBJoypad::DPad dpad_bit)
{
    if (is_dpad_selected())
        joypad_input |= dpad_bit;
}

void Joypad::unset_key(GBJoypad::Buttons buttons_bit)
{
    if (is_buttons_selected())
        joypad_input |= buttons_bit;
}
