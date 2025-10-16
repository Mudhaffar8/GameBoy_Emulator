#include "interrupts.hpp"

void GbInterrupts::request_interrupt(Memory& mem, Interrupts interrupt)
{
    uint8_t& interrupt_flag = mem.read_byte_ref(INTERRUPT_FLAG);
    interrupt_flag |= static_cast<uint8_t>(interrupt);
}

inline bool GbInterrupts::is_interrupt_queued(Memory& mem, Interrupts interrupt)
{
    return mem.read_byte(INTERRUPT_FLAG) & mem.read_byte(INTERRUPT_ENABLE) & static_cast<uint8_t>(interrupt);
}

inline bool GbInterrupts::is_interrupt_requested(Memory& mem, Interrupts interrupt)
{
    return mem.read_byte(INTERRUPT_FLAG) & static_cast<uint8_t>(interrupt);
}

inline bool GbInterrupts::is_interrupt_enabled(Memory& mem, Interrupts interrupt)
{
    return mem.read_byte(INTERRUPT_ENABLE) & static_cast<uint8_t>(interrupt);
}