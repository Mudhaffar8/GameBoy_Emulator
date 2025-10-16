#pragma once

#include <cstdint>

#include "memory.hpp"

namespace GbInterrupts
{
enum class Interrupts : uint8_t 
{
    VBlank = 0x01,
    LCD = 0x02,
    Timer = 0x04,
    Serial = 0x08,
    JoyPad = 0x10
};

void request_interrupt(Memory& mem, Interrupts interrupt);

inline bool is_interrupt_queued(Memory& mem, Interrupts interrupt);
inline bool is_interrupt_requested(Memory& mem, Interrupts interrupt);
inline bool is_interrupt_enabled(Memory& mem, Interrupts interrupt);
}