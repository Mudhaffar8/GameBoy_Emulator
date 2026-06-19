#pragma once

#include <cstdint>

#include "memory.hpp"
#include "interrupts.hpp"

/// @brief Emulates the Game Boy hardware timer system.
class Timer
{
public:
    explicit Timer(Mmu& memory);
    
    /// @brief Advances timer by given number of cycles
    /// @param cycles number of cycles to advance.
    void tick(uint32_t cycles);
    void test(uint32_t cycles);
    
private:
    static constexpr std::array<uint16_t, 4> clock_select_freq { 1024, 16, 64, 256 };

    Mmu& mmu;

    /* Hardware Register References */
    uint8_t& div; // appears as 8-bit reg, incremented every 256 T-cycles
    uint8_t& tima; // Timer Counter: configured using TAC
    uint8_t& tma; // Timer Modulo; When TIMA overflow: reset value to this
    uint8_t& tac; // Controls behaviour for TIMA

    uint16_t div_counter;
    uint16_t timer_counter;
};