#pragma once

#include "memory.hpp"
#include "interrupts.hpp"

#include <cstdint>

/// @brief Represents a 16-bit register that can also be treated as two 8-bit values.
/// @note Implementation assumes your system is little-endian. 
struct RegPair
{
    union
    {
        struct { uint8_t low, high; }; 
        uint16_t r16;
    };

    /* Constructors */
    RegPair() : r16(0) {}
    RegPair(uint16_t _r16) : r16(_r16) {}
    RegPair(uint8_t _high, uint8_t _low) : low(_low), high(_high) {}
};

/// @brief Emulates Game Boy (SM83) CPU.
///
/// Implements instruction fetch/decode/execute and interrupt handling.
class Cpu
{
public:
    explicit Cpu(Mmu& mem);
    
    /// @brief Fetches, decodes and executes one CPU instruction at program counter.
    /// @returns Number of T-cycles taken by the executed instruction.
    uint32_t execute_instruction();
    
    /* Debugging Functions */
    void test(); // Useful for testing one thing at a time

    void print_registers() const;
    void print_flags() const;

private:
    Mmu& mem;
    
    /// @brief CPU flag bitmasks for Flags Register.
    enum class Flags : uint8_t
    {
        Zero = 0x80,
        Subtraction = 0x40,
        HalfCarry = 0x20,
        Carry = 0x10
    };

    /* CPU Registers */
    // 16-bit Register pairs
    RegPair AF;
    RegPair BC;
    RegPair DE;
    RegPair HL;

    // Special case Registers 
    uint8_t& A = AF.high; // Accumulator
    uint8_t& F = AF.low; // Flags

    // 16-bit Registers
    uint16_t PC; // Program Counter
    uint16_t SP; // Stack Pointer

    /* Interrupt handling Register/variables */
    uint8_t IR = 0; // 8-bit Instruction Register
    bool IME = false; // Interrupt Master Enable
    bool is_halted = false; 

    /* Cycle counting (In T-cycles)*/
    uint32_t ticks = 0;

    /* Instruction Execution */
    void cb_execute();

    /* Flag methods */
    inline void set_flag(Flags f, bool cond);
    bool check_flag(Flags f) const;

    /// @brief Checks if a certain CPU flag is set/unset.
    /// @param cond 2-bit value repr. the condition code. 
    /// @returns true if condition is true.
    /// @throws `std::runtime_error` If cond is > 3.
    bool check_condition_code(int cond);

    /* Register Getter Methods */

    /// @brief Retrieves a 8-bit register value/reference based on index.
    /// @param cond 3-bit value repr. the register index.
    /// @returns 8-bit register value/reference.
    /// @throws `std::runtime_error` If cond is > 7.
    uint8_t get_reg(int index);
    uint8_t& get_reg_ref(int index);

    /// @brief Retrieves a 16-bit register value/reference based on index.
    /// @param cond 2-bit value repr. the register index.
    /// @returns 16-bit register value/reference.
    /// @throws `std::runtime_error` If cond is > 3.
    uint16_t get_reg16(int index);
    uint16_t& get_reg16_ref(int index);

    /// @brief Advances PC by 2 and retrieves next 2 bytes in memory.
    /// @returns 16-bit value in little-endian.
    inline uint16_t read_next16();

    /* CPU Instructions */
    void invalid_opcode() const;

    // 8-bit Arithmetic/Logic Operations
    void add_a(uint8_t byte); 
    void adc_a(uint8_t byte);

    void sub_a(uint8_t byte); 
    void sbc_a(uint8_t byte);

    void cp_a(uint8_t reg8);

    void inc_r8(uint8_t& reg8);
    void dec_r8(uint8_t& reg8);

    void and_a(uint8_t reg8);
    void or_a(uint8_t reg8);
    void xor_a(uint8_t reg8);

    // 16-bit Arithmetic/Logic Operations
    void add_hl_r16(uint16_t& r16);

    // 16-bit Load Instructions
    void ld_r16_n16(uint16_t& reg16);

    // Stack Operations
    void push_r16(uint16_t reg16);
    void pop_r16(uint16_t& reg16);

    inline void call_n16(uint16_t nn);

    // Bit manipulation
    void swap_r8(uint8_t& reg8);

    void set_u3_r8(uint8_t& reg8, uint8_t bit_index);
    void bit_u3_r8(uint8_t& reg8, uint8_t bit_index);
    void res_u3_r8(uint8_t& reg8, uint8_t bit_index);

    void sla_r8(uint8_t& reg8);
    void sra_r8(uint8_t& reg8);
    void srl_r8(uint8_t& reg8);

    void rl_r8(uint8_t& reg8); 
    void rr_r8(uint8_t& reg8); 

    void rlc_r8(uint8_t& reg8);
    void rrc_r8(uint8_t& reg8);

    // Miscellaneous 
    inline void daa();

    // Interrupt Handling
    void check_interrupts();
    void handle_interrupt();
};
