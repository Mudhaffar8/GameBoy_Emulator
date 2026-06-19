#include "interrupts.hpp"

inline bool GBInterrupts::is_interrupt_requested(Mmu& mem, Interrupts interrupt)
{
    return mem.read_byte(INTERRUPT_FLAG) & static_cast<uint8_t>(interrupt);
}

inline bool GBInterrupts::is_interrupt_enabled(Mmu& mem, Interrupts interrupt)
{
    return mem.read_byte(INTERRUPT_ENABLE) & static_cast<uint8_t>(interrupt);
}