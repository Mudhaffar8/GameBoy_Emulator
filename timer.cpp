#include "timer.hpp"

#include <iostream>

Timer::Timer(Mmu& _mmu) : 
    mmu(_mmu),
    div(mmu.read_byte_ref(DIVIDER_REGISTER)),
    tima(mmu.read_byte_ref(TIMER_COUNTER)),
    tma(mmu.read_byte_ref(TIMER_MODULO)),
    tac(mmu.read_byte_ref(TIMER_CONTROL))
{}

void Timer::tick(uint32_t cycles)
{
    /*
    div_counter += cycles;
    div = (div_counter & 0xFF00) >> 8; // only upper 8 bits mapped to memory

    bool tima_enable = (tac & 0b100) != 0; 
    if (!tima_enable) return;

    uint32_t clock_freq = clock_select_freq[tac & 0b11]; // find rate TIMA reg is incremented
    uint16_t bit_n = div_counter & clock_freq;
    */
    div_counter += cycles;

    if (div_counter >= 256) 
    { 
        ++div; 
        div_counter %= 256;
    }

    bool tima_enable = (tac & 0b100) != 0;
    uint32_t clock_freq = clock_select_freq[tac & 0b11];

    if (!tima_enable) return;

    // Implementation may probably need to be fixed
    for (int i = 0; i < cycles; ++i)
    {
        ++timer_counter;

        if (timer_counter >= clock_freq)
        {
            if (tima == 0xFF)
            {
                tima = tma;
                GBInterrupts::request_interrupt(mmu, Interrupts::Timer);
            } 
            else ++tima;

            timer_counter %= clock_freq;
        }
    }
}

void Timer::test(uint32_t cycles)
{
    tick(cycles);

    std::cout << "Divider Register: " << div_counter << '\n';
    std::cout << "TIMA Register: " << +tima << '\n';
    std::cout << "TMA Register: " << +tma << '\n';
    std::cout << "TAC Register: " << +tac << '\n';
}