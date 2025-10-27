#include "interrupts.hpp"

void GBInterrupts::request_interrupt(Mmu& mem, Interrupts interrupt)
{
    uint8_t& interrupt_flag = mem.read_byte_ref(INTERRUPT_FLAG);
    interrupt_flag |= static_cast<uint8_t>(interrupt);
}

inline bool GBInterrupts::is_interrupt_queued(Mmu& mem, Interrupts interrupt)
{
    return mem.read_byte(INTERRUPT_FLAG) & mem.read_byte(INTERRUPT_ENABLE) & static_cast<uint8_t>(interrupt);
}

inline bool GBInterrupts::is_interrupt_requested(Mmu& mem, Interrupts interrupt)
{
    return mem.read_byte(INTERRUPT_FLAG) & static_cast<uint8_t>(interrupt);
}

inline bool GBInterrupts::is_interrupt_enabled(Mmu& mem, Interrupts interrupt)
{
    return mem.read_byte(INTERRUPT_ENABLE) & static_cast<uint8_t>(interrupt);
}