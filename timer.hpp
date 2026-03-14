#pragma once

#include <cstdint>

#include "memory.hpp"
#include "interrupts.hpp"

/// @brief Emulates the Game Boy hardware timer system.
class Timer
{
public:
    Timer(Mmu& memory);
    
    /// @brief Advances timer by given number of cycles
    /// @param cycles number of cycles to advance.
    void tick(uint32_t cycles);

private:
    const int clock_select_freq[4] = { 1024, 16, 64, 256 };

    Mmu& mem;

    uint8_t& DIV, TIMA, TMA, TAC;

    uint32_t div_counter, timer_counter;
};