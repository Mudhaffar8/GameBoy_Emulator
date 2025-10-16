#include "timer.hpp"

Timer::Timer(Memory& _mem) : 
    mem(_mem),
    DIV(mem.read_byte_ref(DIVIDER_REGISTER)),
    TIMA(mem.read_byte_ref(TIMER_CONTROL)),
    TMA(mem.read_byte_ref(TIMER_MODULO)),
    TAC(mem.read_byte_ref(TIMER_CONTROL))
{}

void Timer::tick(uint32_t cycles)
{
    div_counter += cycles;

    if (div_counter >= 256) 
    { 
        ++DIV; 
        div_counter %= 256;
    }

    bool tima_enable = (TAC & 0b100) != 0;
    uint32_t clock_freq = clock_select_freq[TAC & 0b11];

    if (!tima_enable) return;

    // Implementation may probably need to be fixed
    for (int i = 0; i < cycles; ++i)
    {
        ++timer_counter;

        if (timer_counter >= clock_freq)
        {
            if (TIMA == 0xFF)
            {
                TIMA = TMA;
                GbInterrupts::request_interrupt(mem, GbInterrupts::Interrupts::Timer);
            } 
            else ++TIMA;

            timer_counter %= clock_freq;
        }
    }
}