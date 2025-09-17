#pragma once

#include "memory.hpp"

#include <cstdint>

enum class Interrupts
{
    VBlank = 0x01,
    LCD = 0x02,
    Timer = 0x04,
    Serial = 0x08,
    JoyPad = 0x10
};

struct RegPair
{
    union
    {
        struct { uint8_t low, high; }; 
        uint16_t r16;
    };

    RegPair() : r16(0) {}
    RegPair(uint16_t _r16) : r16(_r16) {}
    RegPair(uint8_t _low, uint8_t _high) : low(_low), high(_high) {}
};

class Cpu
{
public:
    Cpu();

    void test();

private:
    Memory* mem;
    
    enum class Flags 
    {
        Zero = 0x80,
        Subtraction =  0x40,
        Carry = 0x20,
        HalfCarry = 0x10
    };

    RegPair AF, BC, DE, HL;

    // Special Registers
    uint8_t& A = AF.high; // Accumulator
    uint8_t& F = AF.low;

    // 16-bit Program Counter and Stack pointer
    uint16_t PC;
    uint16_t SP;

    // 8-bit Instruction Register
    uint8_t IR;

    // Interrupt Master Enable
    // Write Only
    bool IME; // Unset when Game Starts Running
    bool is_halted; // unset

    void execute_instruction();

    // Helper methods
    inline void set_flag(Flags f, bool cond);
    bool check_flag(Flags f) const;
    void print_flags() const;

    inline bool check_carry(uint8_t n1, uint8_t n2);
    inline bool check_carry(uint16_t n1, uint16_t n2);
    
    inline bool check_half_carry(uint8_t n1, uint8_t n2);
    inline bool check_half_carry(uint16_t n1, uint16_t n2);

    inline bool check_borrow(uint8_t n1, uint8_t n2);
    inline bool check_half_borrow(uint8_t n1, uint8_t n2);

    bool check_condition_code(uint8_t cond);
    uint8_t get_reg(uint8_t index);
    uint8_t& get_reg_ref(uint8_t index);

    uint16_t read_next16();

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
    void add_sp_e8();
    void add_hl_r16(uint16_t& r16);

    // Load Instructions
    void ld_r16_n16(uint16_t& reg16);

    // Branching
    void jp_hl();
    void jp_cc_n16();
    void jp_n16();

    void jr_e8();
    void jr_cc_e8();

    inline void ret();
    void ret_cc();
    void reti();

    inline void call_nn();
    void call_cc_nn();

    // Bit manipulation
    void swap_r8(uint8_t& reg8);

    void set_u3_r8(uint8_t& reg8);
    void bit_u3_r8(uint8_t& reg8);
    void res_u3_r8(uint8_t& reg8);

    void sra_r8(uint8_t& reg8);
    void sra_hl();

    void srl_r8(uint8_t& reg8);

    void sla_r8(uint8_t& reg8);
    void sla_hl();

    void rr_r8(uint8_t& reg8);
    void rl_r8(uint8_t& reg8);

    void rlc_r8(uint8_t& reg8);
    void rrc_r8(uint8_t& reg8);

    // Miscellaneous 
    inline void daa();
    void stop();
    void nop() const;
    void di();
    void ei();
    void halt();
};
