#pragma once

#include <cstdint>

#include "memory.hpp"

namespace GBInterrupts
{
enum class Interrupts : uint8_t 
{
    VBlank = 0x01,
    LCD = 0x02,
    Timer = 0x04,
    Serial = 0x08,
    JoyPad = 0x10
};

void request_interrupt(Mmu& mem, Interrupts interrupt);

inline bool is_interrupt_queued(Mmu& mem, Interrupts interrupt);
inline bool is_interrupt_requested(Mmu& mem, Interrupts interrupt);
inline bool is_interrupt_enabled(Mmu& mem, Interrupts interrupt);
}