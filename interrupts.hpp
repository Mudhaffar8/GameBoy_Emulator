#pragma once

#include <cstdint>

#include "memory.hpp"

/// @brief Interrupt source bitmasks for IF and IE registers.
enum class Interrupts : uint8_t 
{
    VBlank = 0x01,
    LCD = 0x02,
    Timer = 0x04,
    Serial = 0x08,
    JoyPad = 0x10
};

namespace GBInterrupts
{
    inline void request_interrupt(Mmu& mem, Interrupts interrupt)
    {
        uint8_t& interrupt_flag = mem.read_byte_ref(INTERRUPT_FLAG);
        interrupt_flag |= static_cast<uint8_t>(interrupt);
    }

    inline void unset_interrupt(Mmu& mem, Interrupts interrupt)
    {
        uint8_t& interrupt_flag = mem.read_byte_ref(INTERRUPT_FLAG);
        interrupt_flag &= ~static_cast<uint8_t>(interrupt);
    }

    /* Checking Interrupts */
    inline bool is_interrupt_queued(Mmu& mem, Interrupts interrupt) // Checks if an interrupt is both requested and enabled.
    {
        return mem.read_byte(INTERRUPT_FLAG) & mem.read_byte(INTERRUPT_ENABLE) & static_cast<uint8_t>(interrupt);
    }
    inline bool is_interrupt_requested(Mmu& mem, Interrupts interrupt);
    inline bool is_interrupt_enabled(Mmu& mem, Interrupts interrupt);
}