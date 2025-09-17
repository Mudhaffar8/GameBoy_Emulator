#include "cpu.hpp"

#include <iostream>
#include <cassert>
#include <cstring>

const uint8_t CB_U3_BITMASK = 0b00111000;
const uint8_t CB_OP_BITMASK = 0b00000111;

const uint8_t MSB_BITMASK = 0x80;
const uint8_t LSB_BITMASK = 0x01;

const uint8_t CC_BITMASK = 0b00011000;
const uint8_t CC_BITSHIFT_RIGHT = 2;

const uint8_t LOW_NIBBLE_MASK = 0x0F;
const uint8_t HIGH_NIBBLE_MASK = 0xF0;

Cpu::Cpu()
{
    F = 0;
}

// For Testing Purposes
void Cpu::test()
{
    // 0b10001xxx
    // IR = 0b10001010; // ADC A, D

    // A = 0x00;
    // reg8[2] = 0x02;

    // bit_u3_r8();

    // print_flags();

    // bool flag_z = check_flag(Flags::Zero);
    // bool flag_s = check_flag(Flags::Subtraction);
    // bool flag_c = check_flag(Flags::Carry);
    // bool flag_hc = check_flag(Flags::HalfCarry);

    // std::cout << static_cast<int>(A) << std::endl;

    // assert(flag_z == false);
    // assert(flag_s == false);
    // assert(flag_c == false);
    // assert(flag_hc == true);

}


/* Flag Functions - All Flag Functions work */
inline void Cpu::set_flag(Flags f, bool cond)
{
    F = (cond) ? (F | static_cast<uint8_t>(f)) : (F & ~static_cast<uint8_t>(f));
}

bool Cpu::check_flag(Flags f) const
{
    return (F & static_cast<uint8_t>(f)) != 0;
}

void Cpu::print_flags() const
{
    std::cout << "Z: " << static_cast<int>(check_flag(Flags::Zero)) << ", ";
    std::cout << "N: " << static_cast<int>(check_flag(Flags::Subtraction)) << ", ";
    std::cout << "C: " << static_cast<int>(check_flag(Flags::Carry)) << ", ";
    std::cout << "H: " << static_cast<int>(check_flag(Flags::HalfCarry)) << ", ";

    for (uint8_t i = 0; i < 8; i++)
        std::cout << (((0x80 >> i) & F) != 0);
    
    std::cout << '\n';
}


/* Helper methods */
inline bool Cpu::check_carry(uint8_t n1, uint8_t n2)
{
    return (n1 + n2) > 0xFF;
}

inline bool Cpu::check_carry(uint16_t n1, uint16_t n2)
{
    return (n1 + n2) > 0xFFFF;
}

inline bool Cpu::check_half_carry(uint8_t n1, uint8_t n2)
{
    return ((n1 & 0xF) + (n2 & 0xF)) > 0xF;
}

inline bool Cpu::check_half_carry(uint16_t n1, uint16_t n2)
{
    return ((n1 & 0xFFF) + (n2 & 0xFFF)) > 0xFFF;
}

inline bool Cpu::check_borrow(uint8_t minuend, uint8_t subtrahend)
{
    return minuend < subtrahend;
}

inline bool Cpu::check_half_borrow(uint8_t minuend, uint8_t subtrahend)
{
    return (subtrahend & 0x0F) > (minuend & 0x0F);
}

uint16_t Cpu::read_next16()
{
    uint8_t low = mem->read_byte(++PC);
    uint8_t high = mem->read_byte(++PC);

    return (high << 8) | low;
}

bool Cpu::check_condition_code(uint8_t cond) 
{ 
    switch(cond)
    {
    case 0:
        return !check_flag(Flags::Zero);

    case 1:
        return check_flag(Flags::Zero);

    case 2:
        return !check_flag(Flags::Carry);

    case 3:
        return check_flag(Flags::Carry);
    
    default:
        std::cerr << "Invalid condition code " << +cond << '\n';
        break;
    }

    return false;
}

uint8_t Cpu::get_reg(uint8_t index)
{
    switch (index)
    {
    case 0:
        return BC.high;
    case 1:
        return BC.low;
    case 2:
        return DE.high;
    case 3:
        return DE.low;
    case 4:
        return HL.high;
    case 5:
        return HL.low;  
    case 6:
        return mem->read_byte(HL.r16);
    case 7:
        return AF.high;
    }
}

uint8_t& Cpu::get_reg_ref(uint8_t index)
{
    switch (index)
    {
    case 0:
        return BC.high;
    case 1:
        return BC.low;
    case 2:
        return DE.high;
    case 3:
        return DE.low;
    case 4:
        return HL.high;
    case 5:
        return HL.low;   
    case 7:
        return AF.high;
    }
}

void Cpu::execute_instruction()
{
    if (is_halted) return;

    IR = mem->read_byte(PC);

    switch (IR)
    {
    // Good
    case 0x00: 
        nop();
        break;

    // LD BC, n16
    // Good
    case 0x01:
        ld_r16_n16(BC.r16);
        break;
    
    // LD [BC], A
    // Good
    case 0x02:
        mem->write_byte(A, BC.r16);
        break;
    
    // Good
    case 0x03:
        ++BC.r16;
        break;
    
    // Good
    case 0x04:
        inc_r8(BC.high);
        break;
    
    // Good
    case 0x05:
        dec_r8(BC.high);
        break;

    // LD B, n8
    // Good
    case 0x06:
        BC.high = mem->read_byte(++PC);
        break;
    
    // RLCA
    // Good
    case 0x07:
        rlc_r8(A);
        break;

    // LD [a16], SP
    // Copy SP & $FF at address n16 and SP >> 8 at address n16 + 1.
    // Good
    case 0x08:
        uint16_t n16 = read_next16();

        mem->write_byte(SP & 0xFF, n16);
        mem->write_byte(SP >> 8, n16 + 1);
        break;
    
    // ADD HL, BC
    // Good
    case 0x09:
        add_hl_r16(BC.r16);
        break;
    
    // LD A, [BC]
    // Good
    case 0x0A:
        A = mem->read_byte(BC.r16);
        break;
    
    // Good
    case 0x0B:
        --BC.r16;
        break;
    
    // Good
    case 0x0C:
        inc_r8(BC.low);
        break;
    
    // Good
    case 0x0D:
        dec_r8(BC.low);
        break;

    // LD C, n8
    // Good
    case 0x0E:
        BC.low = mem->read_byte(++PC);
        break;
    
    // RRCA
    // Good
    case 0x0F:
        rrc_r8(A);
        break;
    
    // Good
    case 0x10:
        stop();
        break;

    // LD DE, n16
    // Good
    case 0x11:
        ld_r16_n16(DE.r16);
        break;
    
    // LD [DE], A
    // Good
    case 0x12:
        mem->write_byte(A, DE.r16);
        break;
    
    // Good
    case 0x13:
        ++DE.r16;
        break;
    
    // Good
    case 0x14:
        inc_r8(DE.high);
        break;
    
    // Good
    case 0x15:
        dec_r8(DE.high);
        break;

    // LD D, n8
    // Good
    case 0x16:
        DE.high = mem->read_byte(++PC);
        break;
    
    // RLA
    // Good
    case 0x17:
        rl_r8(A);
        break;

    // JR e8
    // Good
    case 0x18:
        jr_e8();
        break;
    
    // ADD HL, DE
    // Good
    case 0x19:
        add_hl_r16(DE.r16);
        break;
    
    // LD A, [DE]
    // Good
    case 0x1A:
        A = mem->read_byte(DE.r16);
        break;
    
    // Good
    case 0x1B:
        --DE.r16;
        break;
    
    // Good
    case 0x1C:
        inc_r8(DE.low);
        break;
    
    // Good
    case 0x1D:
        dec_r8(DE.low);
        break;

    // LD E, n8
    // Good
    case 0x1E:
        DE.low = mem->read_byte(++PC);
        break;
    
    // RRA
    // Good
    case 0x1F:
        rr_r8(A);
        break;
    
    // JR NZ, e8
    // Good
    case 0x20:
        jr_cc_e8();
        break;

    // LD HL, n16
    // Good
    case 0x21:
        ld_r16_n16(HL.r16);
        break;
    
    // LD [HL+], A
    // Good
    case 0x22:
        mem->write_byte(A, HL.r16);
        ++HL.r16;
        break;
    
    // Good
    case 0x23:
        ++HL.r16;
        break;
    
    // Good
    case 0x24:
        inc_r8(HL.high);
        break;
    
    // Good
    case 0x25:
        dec_r8(HL.high);
        break;
    
    // LD H, n8
    // Good
    case 0x26:
        HL.high = mem->read_byte(++PC);
        break;
    
    // DAA
    // Good
    case 0x27:
        daa();
        break;

    // JR Z, e8
    // Good
    case 0x28:
        jr_cc_e8();
        break;
    
    // ADD HL, HL
    case 0x29:
        add_hl_r16(HL.r16);
        break;
    
    // LD A, [HL+]
    // Good
    case 0x2A:
        A = mem->read_byte(HL.r16);
        ++HL.r16;
        break;
    
    // Good
    case 0x2B:
        --HL.r16;
        break;
    
    // INC L
    // Good
    case 0x2C:
        inc_r8(HL.low);
        break;

    // DEC L
    // Good
    case 0x2D:
        dec_r8(HL.low);
        break;

    // LD L, n8
    // Good
    case 0x2E:
        HL.low = mem->read_byte(++PC);
        break;
    
    // CPL
    // Good
    case 0x2F:
        A = ~A;

        set_flag(Flags::Subtraction, true);
        set_flag(Flags::HalfCarry, true);
        break;

    // JR NC, e8
    // Good
    case 0x30:
        jr_cc_e8();
        break;

    // LD SP, n16
    // Good
    case 0x31:
        ld_r16_n16(SP);
        break;
    
    // LD [HL-], A
    // Good
    case 0x32:
        mem->write_byte(A, HL.r16);
        --HL.r16;
        break;
    
    // INC SP
    // Good
    case 0x33:
        ++SP;
        break;
    
    // INC [HL]
    // Good
    case 0x34:
        inc_r8(mem->read_byte_ref(HL.r16));
        break;
    
    // DEC [HL]
    // Good
    case 0x35:
        dec_r8(mem->read_byte_ref(HL.r16));
        break;

    // LD [HL], n8
    // 2 cycles
    // Good
    case 0x36:
        uint8_t n8 = mem->read_byte(++PC);
        mem->write_byte(HL.r16, n8);
        break;
    
    // SCF
    // Good
    case 0x37:
        set_flag(Flags::HalfCarry, false);
        set_flag(Flags::Subtraction, false);
        set_flag(Flags::Carry, true);
        break;

    // JR C, e8
    // Good
    case 0x38:
        jr_cc_e8();
        break;
    
    // ADD HL, SP
    // Good
    case 0x39:
        add_hl_r16(SP);
        break;
    
    // LD A, [HL-]
    // Good
    case 0x3A:
        A = mem->read_byte(HL.r16);
        --HL.r16;
        break;

    // DEC SP
    // Good
    case 0x3B:
        --SP;
        break;

    // Good
    case 0x3C:
        inc_r8(A);
        break;

    // Good
    case 0x3D:
        dec_r8(A);
        break;

    // LD A, n8
    // Good
    case 0x3E:
        A = mem->read_byte(++PC);
        break;
    
    // CCF
    // Good
    case 0x3F:
        set_flag(Flags::HalfCarry, false);
        set_flag(Flags::Subtraction, false);
        set_flag(Flags::Carry, !check_flag(Flags::Carry)); 
        break;
    
    // LD B, B
    case 0x40:
        BC.high = BC.high;
        break;
    
    // LD B, C
    case 0x41:
        BC.high = BC.low;
        break;

    // LD B, D
    case 0x42:
        BC.high = DE.high;
        break;
    
    // LD B, E
    case 0x43:
        BC.high = DE.low;
        break;
    
    // LD B, H
    case 0x44:
        BC.high = HL.high;
        break;
    
    // LD B, L
    case 0x45:
        BC.high = HL.low;
        break;

    // LD B, [HL]
    case 0x46:
        BC.high = mem->read_byte(HL.r16);
        break;
    
    // LD B, A
    case 0x47:
        BC.high = A;
        break;

    // LD C, B
    case 0x48:
        BC.low = BC.high;
        break;
    
    // LD C, C
    case 0x49:
        BC.low = BC.low;
        break;

    // LD C, D
    case 0x4A:
        BC.low = DE.high;
        break;
    
    // LD C, E
    case 0x4B:
        BC.low = DE.low;
        break;
    
    // LD C, H
    case 0x4C:
        BC.low = HL.high;
        break;
    
    // LD C, L
    case 0x4D:
        BC.low = HL.low;
        break;

    // LD C, [HL]
    case 0x4E:
        BC.low = mem->read_byte(HL.r16);
        break;
    
    // LD C, A
    case 0x4F:
        BC.low = A;
        break;

    // LD D, B
    case 0x50:
        DE.high = BC.high;
        break;
    
    // LD D, C
    case 0x51:
        DE.high = BC.low;
        break;

    // LD D, D
    case 0x52:
        DE.high = DE.high;
        break;
    
    // LD D, E
    case 0x53:
        DE.high = DE.low;
        break;
    
    // LD D, H
    case 0x54:
        DE.high = HL.high;
        break;
    
    // LD D, L
    case 0x55:
        DE.high = HL.low;
        break;

    // LD D, [HL]
    case 0x56:
        DE.high = mem->read_byte(HL.r16);
        break;
    
    // LD D, A
    case 0x57:
        DE.high = A;
        break;

    // LD E, B
    case 0x58:
        DE.low = BC.high;
        break;
    
    // LD C, C
    case 0x59:
        BC.low = BC.low;
        break;

    // LD C, D
    case 0x5A:
        BC.low = DE.high;
        break;
    
    // LD C, E
    case 0x5B:
        BC.low = DE.low;
        break;
    
    // LD C, H
    case 0x5C:
        BC.low = HL.high;
        break;
    
    // LD C, L
    case 0x5D:
        BC.low = HL.low;
        break;

    // LD C, [HL]
    case 0x5E:
        BC.low = mem->read_byte(HL.r16);
        break;
    
    // LD C, A
    case 0x5F:
        BC.low = A;
        break;

    // LD H, B
    case 0x60:
        HL.high = BC.high;
        break;
    
    // LD H, C
    case 0x61:
        HL.high = BC.low;
        break;

    // LD H, D
    case 0x62:
        HL.high = DE.high;
        break;
    
    // LD H, E
    case 0x63:
        HL.high = DE.low;
        break;
    
    // LD H, H
    case 0x64:
        HL.high = HL.high;
        break;
    
    // LD HL, L
    case 0x65:
        HL.high = HL.low;
        break;

    // lD
    case 0x66:
        HL.high = mem->read_byte(HL.r16);
        break;
    
    // LD H, A
    case 0x67:
        HL.high = A;
        break;

    // LD L, B
    case 0x68:
        HL.low = BC.high;
        break;
    
    // LD L, C
    case 0x69:
        HL.low = BC.low;
        break;

    // LD L, D
    case 0x6A:
        HL.low = DE.high;
        break;
    
    // LD L, E
    case 0x6B:
        HL.low = DE.low;
        break;
    
    // LD L, H
    case 0x6C:
        HL.low = HL.high;
        break;
    
    // LD L, L
    case 0x6D:
        HL.low = HL.low;
        break;

    // LD L, [HL]
    case 0x6E:
        HL.low = mem->read_byte(HL.r16);
        break;
    
    // LD L, A
    case 0x6F:
        HL.low = A;
        break;

    // LD D, B
    case 0x70:
        DE.high = BC.high;
        break;
    
    // LD D, C
    case 0x71:
        DE.high = BC.low;
        break;

    // LD D, D
    case 0x72:
        DE.high = DE.high;
        break;
    
    // LD D, E
    case 0x73:
        DE.high = DE.low;
        break;
    
    // LD D, H
    case 0x74:
        DE.high = HL.high;
        break;
    
    // LD D, L
    case 0x75:
        DE.high = HL.low;
        break;

    // LD D, [HL]
    case 0x76:
        DE.high = mem->read_byte(HL.r16);
        break;
    
    // LD D, A
    case 0x77:
        DE.high = A;
        break;

    // LD E, B
    case 0x78:
        DE.low = BC.high;
        break;
    
    // LD C, C
    case 0x79:
        BC.low = BC.low;
        break;

    // LD C, D
    case 0x7A:
        BC.low = DE.high;
        break;
    
    // LD C, E
    case 0x7B:
        BC.low = DE.low;
        break;
    
    // LD C, H
    case 0x7C:
        BC.low = HL.high;
        break;
    
    // LD C, L
    case 0x7D:
        BC.low = HL.low;
        break;

    // LD C, [HL]
    case 0x7E:
        BC.low = mem->read_byte(HL.r16);
        break;
    
    // LD C, A
    case 0x7F:
        BC.low = A;
        break;

    default:
        break;
    }

    ++PC;
}


/* CPU Instructions */

void Cpu::invalid_opcode() const
{
    std::cout << "Invalid Opcode: " << std::hex << +IR << '\n';
}

// 0x00
// -> 1 cycle
// -> 1 byte
void Cpu::nop() const {}

// 0b10001xxx ~
// -> 1 cycle
// -> 1 
void Cpu::adc_a(uint8_t reg8)
{
    int sum = A + reg8 + static_cast<int>(check_flag(Flags::Carry));

    set_flag(Flags::Carry, sum > 0xFF);
    set_flag(Flags::HalfCarry, sum > 0xFF);

    A = sum;

    set_flag(Flags::Zero, A == 0);
    set_flag(Flags::Subtraction, false);
}


void Cpu::add_a(uint8_t reg8)
{
    set_flag(Flags::Carry, check_carry(reg8, A));
    set_flag(Flags::HalfCarry, check_half_carry(reg8, A));

    A += reg8;

    set_flag(Flags::Zero, A == 0);
    set_flag(Flags::Subtraction, false);
}


void Cpu::sub_a(uint8_t reg8)
{
    set_flag(Flags::Carry, check_borrow(reg8, A));
    set_flag(Flags::HalfCarry, check_half_borrow(reg8, A));

    A -= reg8;

    set_flag(Flags::Zero, A == 0);
    set_flag(Flags::Subtraction, true);
}


void Cpu::sbc_a(uint8_t reg8)
{
    int A_plus_carry = A + static_cast<int>(check_flag(Flags::Carry));
    int res = A_plus_carry - reg8;

    set_flag(Flags::Carry, check_borrow(A, reg8));
    set_flag(Flags::HalfCarry, check_half_borrow(A, reg8));

    A = res;

    set_flag(Flags::Zero, A == 0);
    set_flag(Flags::Subtraction, true);
}

void Cpu::cp_a(uint8_t reg8) 
{
    set_flag(Flags::Carry, check_borrow(reg8, A));
    set_flag(Flags::HalfCarry, check_half_borrow(reg8, A));

    uint8_t res = A - reg8;

    set_flag(Flags::Zero, res == 0);
    set_flag(Flags::Subtraction, true);
}


//  0b00xxx100 ~ 
void Cpu::inc_r8(uint8_t& reg8)
{
    set_flag(Flags::HalfCarry, check_half_carry(reg8, 1));

    reg8++;

    set_flag(Flags::Zero, reg8 == 0);
    set_flag(Flags::Subtraction, false);
}

// 0b00xxx101
void Cpu::dec_r8(uint8_t& reg8) 
{
    set_flag(Flags::HalfCarry, check_half_borrow(reg8, 1));

    --reg8;

    set_flag(Flags::Zero, reg8 == 0);
    set_flag(Flags::Subtraction, true);
}


void Cpu::and_a(uint8_t reg8)
{
    int i = IR & 0x07;

    A &= reg8;

    set_flag(Flags::Zero, A == 0);
    set_flag(Flags::HalfCarry, true);
    set_flag(Flags::Subtraction, false);
    set_flag(Flags::Carry, false);
}

void Cpu::and_a(uint8_t byte)
{

    A &= byte;

    set_flag(Flags::Zero, A == 0);
    set_flag(Flags::HalfCarry, true);
    set_flag(Flags::Subtraction, false);
    set_flag(Flags::Carry, false);
}


// (Checked)
void Cpu::or_a(uint8_t byte)
{
    A |= byte;

    set_flag(Flags::Zero, A == 0);
    set_flag(Flags::HalfCarry, true);
    set_flag(Flags::Subtraction, false);
    set_flag(Flags::Carry, false);
}

void Cpu::xor_a(uint8_t byte)
{
    A ^= byte;

    set_flag(Flags::Zero, A == 0);
    set_flag(Flags::HalfCarry, true);
    set_flag(Flags::Subtraction, false);
    set_flag(Flags::Carry, false);
}


/* 16-bit Arithmetic/Logic Operations */
// 0b11101000 + signed 8-bit integer
void Cpu::add_sp_e8()
{
    int8_t val = static_cast<int8_t>(mem->read_byte(++PC));

    set_flag(Flags::HalfCarry, check_carry(SP, val)); // Potential Issue
    set_flag(Flags::Carry, check_half_carry(SP, val)); // Potential Issue

    SP += val;

    set_flag(Flags::Zero, false);
    set_flag(Flags::Subtraction, false);
}

// 0b00xx1001, xx = register
// 2 cycles
// 1 byte
void Cpu::add_hl_r16(uint16_t& r16)
{
    set_flag(Flags::HalfCarry, check_half_carry(HL.r16, r16));
    set_flag(Flags::Carry, check_carry(HL.r16, r16));

    HL.r16 += r16;

    set_flag(Flags::Subtraction, false);
}

/* Branching */
// 1 cycle
// 1 byte
void Cpu::jp_hl()
{
    PC = HL.r16;
}

// 0b110xx010 + LSB + MSB -> xx = condition code
// 3 bytes
// 4 taken/ 3 untaken cycles
void Cpu::jp_cc_n16()
{
    uint8_t cc = (IR & CC_BITMASK) >> CC_BITSHIFT_RIGHT;
    if (check_condition_code(cc))
    {
        jp_n16();
    }
}

// 0b11000011/0xC3
// 3 bytes
// 4 cycles
void Cpu::jp_n16() 
{
    PC = read_next16();
}


void Cpu::jr_cc_e8()
{
    uint8_t cc = (IR & CC_BITMASK) >> CC_BITSHIFT_RIGHT;
    if (check_condition_code(cc))
    {
        jr_e8();
    }
}

// 0b00011000/0x18 + int8
// 2 bytes
// 3 cycles
void Cpu::jr_e8()
{
    uint8_t byte = mem->read_byte(++PC);
    PC += static_cast<int8_t>(byte);
}

// 4 cycles
// 1 byte
inline void Cpu::ret() 
{
    uint8_t lsb = mem->read_byte(++SP);
    uint8_t msb = mem->read_byte(++SP);

    PC = (msb << 8) | lsb;
}

// 0b110xx000
// 5 true/ 2 false
void Cpu::ret_cc() 
{
    uint8_t cond = (IR & CC_BITMASK) >> CC_BITSHIFT_RIGHT;
    if (check_condition_code(cond))
    {
        ret();
    }
}

void Cpu::reti() 
{
    ret();
    IME = true;
}

// 6 cycles
// 3 bytes
inline void Cpu::call_nn() 
{
    uint16_t addr = read_next16();

    mem->write_byte(PC & LOW_NIBBLE_MASK, SP--);
    mem->write_byte(PC & HIGH_NIBBLE_MASK, SP--);

    PC = addr;
}

// 0b110xx100 + LSB + MSB
// 3 bytes
// 6/3 cycles
void Cpu::call_cc_nn() 
{
    uint8_t cc = (IR & CC_BITMASK) >> CC_BITSHIFT_RIGHT;
    if (check_condition_code(cc))
    {
        call_nn();
    }
}

/* Bit operations */
// CB Prefix + 0b00110xxx
void Cpu::swap_r8(uint8_t& reg8)
{
    int i = IR & CB_OP_BITMASK;

    uint8_t low_nibble = reg8 & 0x0F;
    uint8_t hi_nibble = (reg8 & 0xF0) >> 4;

    reg8 = (low_nibble << 4) | hi_nibble;

    set_flag(Flags::Zero, reg8 == 0);
    set_flag(Flags::Subtraction, false);
    set_flag(Flags::HalfCarry, false);
    set_flag(Flags::Carry, false);
}

// CB-Prefix + 0b01yyyxxx - xxx = operand, yyy = bit index
// Checked
void Cpu::bit_u3_r8(uint8_t& reg8)
{
    int i = IR & CB_OP_BITMASK;
    uint8_t bit_index = (IR & CB_U3_BITMASK) >> 3;

    std::cout << "Bit Index: " << static_cast<int>(bit_index) << std::endl;
    std::cout << "Register Index: " << static_cast<int>(i) << std::endl;

    uint8_t bit_to_check = reg8 & (0x1 << bit_index);

    set_flag(Flags::Zero, bit_to_check == 0);
    set_flag(Flags::Subtraction, false);
    set_flag(Flags::HalfCarry, true);
}

// CB-Prefix + 0b10yyyxxx - xxx = operand, yyy = bit index
void Cpu::res_u3_r8(uint8_t& reg8)
{
    int i = IR & CB_OP_BITMASK;
    uint8_t bit_index = (IR & CB_U3_BITMASK) >> 3;

    reg8 &= ~(0x1 << bit_index);
}

// CB Prefix + 0b11yyyxxx - xxx = operand, yyy = bit index
void Cpu::set_u3_r8(uint8_t& reg8)
{
    int i = IR & CB_OP_BITMASK;
    uint8_t bit_index = (IR & CB_U3_BITMASK) >> 3;

    reg8 |= (0x1 << bit_index);
}


void Cpu::ld_r16_n16(uint16_t& reg16)
{
    uint16_t byte = read_next16();

    reg16 = byte;
}

// CB Prefix + 0b00101xxx - xxx = register
// -> 2 bytes + n
// -> 2 cycles
void Cpu::sra_r8(uint8_t& reg8)
{
    int i = IR & CB_OP_BITMASK;

    uint8_t msb = reg8 & 0x80;
    uint8_t lsb = reg8 & 0x01;

    reg8 >>= 1;
    reg8 |= msb;

    set_flag(Flags::Carry, lsb == 1);
    set_flag(Flags::Zero, reg8 == 0);
    set_flag(Flags::Subtraction, false);
    set_flag(Flags::HalfCarry, false);
}

// CB Prefix + 0b00101110
// -> 4 bytes
// -> 2 cycles
void Cpu::sra_hl()
{
    uint8_t val = mem->read_byte(HL.r16);

    uint8_t msb = val & 0x80;
    uint8_t lsb = val & 0x01;

    val >>= 1;
    val |= msb;

    set_flag(Flags::Carry, lsb == 1);
    set_flag(Flags::Zero, val == 0);
    set_flag(Flags::Subtraction, false);
    set_flag(Flags::HalfCarry, false);
}

// CB Prefix + 0b00111xxx/various
// 2 cycles
// 2 bytes
void Cpu::srl_r8(uint8_t& reg8)
{
    set_flag(Flags::Subtraction, false);
    set_flag(Flags::HalfCarry, false);

    int i = IR & CB_OP_BITMASK;

    uint8_t lsb = reg8 & LSB_BITMASK;

    reg8 >>= 1;

    set_flag(Flags::Carry, lsb == 1);
    set_flag(Flags::Zero, reg8 == 0);
}

// CB Prefix + 0b00100xxx - xxx = register
// -> 2 bytes
// -> 2 cycles
void Cpu::sla_r8(uint8_t& reg8)
{
    int i = IR & CB_OP_BITMASK;

    uint8_t msb = reg8 & 0x80;

    reg8 <<= 1;

    set_flag(Flags::Carry, msb == 1);
    set_flag(Flags::Zero, reg8 == 0);
    set_flag(Flags::Subtraction, false);
    set_flag(Flags::HalfCarry, false);
}

// CB Prefix + 0b00100110 - xxx = register
// -> 2 bytes
// -> 4 cycles 0x26
void Cpu::sla_hl()
{
    uint8_t val = mem->read_byte(HL.r16);

    uint8_t msb = val & 0x80;

    val <<= 1;

    mem->write_byte(val, HL.r16);

    set_flag(Flags::Carry, msb == 1);
    set_flag(Flags::Zero, val == 0);
    set_flag(Flags::Subtraction, false);
    set_flag(Flags::HalfCarry, false);
}

// CB Prefix + 0b00011xxx
// C -> [7 -> 0] -> C
// 2 bytes
// 2 cycles
void Cpu::rr_r8(uint8_t& reg8)
{
    set_flag(Flags::Subtraction, false);
    set_flag(Flags::HalfCarry, false);

    int i = IR & CB_OP_BITMASK;

    uint8_t lsb = reg8 & LSB_BITMASK;

    reg8 >>= 1;
    reg8 |= (static_cast<uint8_t>(check_flag(Flags::Carry)) << 7);

    set_flag(Flags::Carry, lsb == 1);
    set_flag(Flags::Zero, reg8 == 0 && i != 7);
}

// CB Prefix + 0b00010xxx
// C <- [7 <- 0] <- C
// 2 bytes
// 2 machine cycles
void Cpu::rl_r8(uint8_t& reg8)
{
    set_flag(Flags::Subtraction, false);
    set_flag(Flags::HalfCarry, false);

    int i = IR & CB_OP_BITMASK;

    uint8_t msb = reg8 & MSB_BITMASK;

    reg8 <<= 1;
    reg8 |= static_cast<uint8_t>(check_flag(Flags::Carry));

    set_flag(Flags::Carry, msb == 1);
    set_flag(Flags::Zero, reg8 == 0 && i != 7);
}

// CB Prefix + 0b00000xxx
// For RLCA: 0x07
// C <- [7 <- 0] <- [7]
// 2 bytes
// 2 cycles
void Cpu::rlc_r8(uint8_t& reg8)
{
    set_flag(Flags::Subtraction, false);
    set_flag(Flags::HalfCarry, false);

    int i = IR & CB_OP_BITMASK;

    uint8_t msb = reg8 & MSB_BITMASK;
    reg8 <<= 1;

    reg8 |= (msb >> 7);

    set_flag(Flags::Carry, msb == 1);
    set_flag(Flags::Zero, reg8 == 0 && i != 7);
}

// CB Prefix + 0b00000xxx
// [0] -> [7 -> 0] -> C
// 2 bytes
// 2 cycles
void Cpu::rrc_r8(uint8_t& reg8)
{
    set_flag(Flags::Subtraction, false);
    set_flag(Flags::HalfCarry, false);

    int i = IR & CB_OP_BITMASK;

    uint8_t lsb = reg8 & LSB_BITMASK;
    reg8 >>= 1;

    reg8 |= lsb;

    set_flag(Flags::Carry, lsb == 1);
    set_flag(Flags::Zero, reg8 == 0 && i != 7);
}

void Cpu::di()
{
    IME = false;
}

void Cpu::ei()
{
    IME = true;
}

void Cpu::halt()
{

}

// 0b00100111/0x27
// 1 cycle
// 1 byte
inline void Cpu::daa()
{
    uint8_t offset = 0;
    bool is_subtraction = check_flag(Flags::Subtraction);

    if ((!is_subtraction && (A & 0x0F > 0x09)) || check_flag(Flags::HalfCarry))
        offset |= 0x06;

    if ((!is_subtraction && A > 0x99) || check_flag(Flags::Carry))
    {
        offset |= 0x60;
        set_flag(Flags::Carry, true); 
    }
    else set_flag(Flags::Carry, false);

    A = (is_subtraction) ? A - offset : A + offset;

    set_flag(Flags::HalfCarry, A == 0);
    set_flag(Flags::Zero, true);
}

// 2 bytes
// 
void Cpu::stop()
{
    // ++PC;
    // uint8_t byte = mem->read_byte(PC);
}

void Cpu::halt()
{
    is_halted = true;
}