#include "cpu.hpp"

#include <iostream>
#include <cassert>
#include <cstring>
#include <stdexcept> // Should get rid of these soon

using Interrupts = GbInterrupts::Interrupts;

constexpr uint8_t CB_U3_BITMASK = 0b111000;
constexpr uint8_t CB_OP_BITMASK = 0b111;

constexpr uint8_t LD_SRC_BITMASK = 0b111;
constexpr uint8_t LD_DST_BITMASK = 0b111000;

constexpr uint8_t ARITHMETIC_OP_BITMASK = 0x07;

constexpr uint8_t MSB_BITMASK = 0x80;
constexpr uint8_t LSB_BITMASK = 0x01;

constexpr uint8_t LOW_NIBBLE_MASK = 0x0F;
constexpr uint8_t HIGH_NIBBLE_MASK = 0xF0;

constexpr uint16_t DMG_AF_INIT = 0x01B0;
constexpr uint16_t DMG_BC_INIT = 0x0013;
constexpr uint16_t DMG_DE_INIT = 0x00D8;
constexpr uint16_t DMG_HL_INIT = 0x014D;
constexpr uint16_t DMG_SP_INIT = HIGH_RAM_END;


Cpu::Cpu(Memory& _mem) :
    mem(_mem),
    AF(DMG_AF_INIT), 
    BC(DMG_BC_INIT), 
    DE(DMG_DE_INIT), 
    HL(DMG_HL_INIT),
    PC(),
    SP(DMG_SP_INIT),
    IR()
{}

void print_reg(uint8_t reg) { std::cout << "0x" << std::hex << +reg << ", "; }
void print_reg(uint16_t reg) { std::cout << "0x" << std::hex << +reg << ", "; }
void print_reg(RegPair reg) 
{ 
    std::cout << std::hex << "High: 0x" << std::hex << +reg.high << ", " << "Low: 0x" << std::hex << +reg.low << ", "; 
}


// For Testing Purposes
void Cpu::test()
{    
    // request_interrupt(mem, Interrupts::LCD);
}


void Cpu::print_registers() const
{
    std::cout << "A: ";
    print_reg(AF.high);
  
    std::cout << "F: ";
    print_reg(AF.low);

    std::cout << "B: ";
    print_reg(BC.high);

    std::cout << "C: ";
    print_reg(BC.low);

    std::cout << "D: ";
    print_reg(DE.high);

    std::cout << "E: ";
    print_reg(DE.low);

    std::cout << "H: ";
    print_reg(HL.high);

    std::cout << "L: ";
    print_reg(HL.low);

    std::cout << "PC: ";
    print_reg(PC);

    std::cout << "SP: ";
    print_reg(SP);

    std::cout << '\n';
}

/* Flag Functions */
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
    std::cout << "H: " << static_cast<int>(check_flag(Flags::HalfCarry)) << ", ";
    std::cout << "C: " << static_cast<int>(check_flag(Flags::Carry)) << ", ";

    for (uint8_t i = 0; i < 8; i++)
        std::cout << (((0x80 >> i) & F) != 0);
    
    std::cout << '\n';
}


/* Helper methods */
inline bool check_carry(uint8_t n1, uint8_t n2)
{
    return (n1 + n2) > 0xFF;
}

inline bool check_carry(uint16_t n1, uint16_t n2)
{
    return (n1 + n2) > 0xFFFF;
}

inline bool check_carry(uint16_t n1, int8_t n2)
{
    return ((n1 & 0xFF) + static_cast<uint8_t>(n2)) > 0xFF;
}

inline bool check_carry(uint8_t n1, uint8_t n2, uint8_t carry)
{
    return (n1 + n2 + carry) > 0xFF;
}

inline bool check_half_carry(uint8_t n1, uint8_t n2)
{
    return ((n1 & LOW_NIBBLE_MASK) + (n2 & LOW_NIBBLE_MASK)) > 0xF;
}

inline bool check_half_carry(uint16_t n1, uint16_t n2)
{
    return ((n1 & 0xFFF) + (n2 & 0xFFF)) > 0xFFF;
}

inline bool check_half_carry(uint8_t n1, uint8_t n2, uint8_t carry)
{
    return ((n1 & LOW_NIBBLE_MASK) + (n2 & LOW_NIBBLE_MASK) + (carry & LOW_NIBBLE_MASK)) > 0xF;
}

inline bool check_half_carry(uint16_t n1, int8_t n2)
{
    return ((n1 & LOW_NIBBLE_MASK) + (n2 & LOW_NIBBLE_MASK)) > 0xF;
}


inline bool check_borrow(uint8_t n1, uint8_t n2)
{
    return n2 > n1;
}

inline bool check_borrow(uint8_t n1, uint8_t n2, uint8_t carry)
{
    return (n2 + carry) > n1;
}

inline bool check_half_borrow(uint8_t n1, uint8_t n2, uint8_t carry)
{
    return ((n2 & LOW_NIBBLE_MASK) + (carry & LOW_NIBBLE_MASK)) > ((n1) & LOW_NIBBLE_MASK);
}

inline bool check_half_borrow(uint8_t n1, uint8_t n2)
{
    return (n2 & LOW_NIBBLE_MASK) > (n1 & LOW_NIBBLE_MASK);
}

inline uint16_t Cpu::read_next16()
{
    uint8_t low = mem.read_byte(PC++);
    uint8_t high = mem.read_byte(PC++);

    return (high << 8) | low;
}

bool Cpu::check_condition_code(int cond) 
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
        std::cerr << "Invalid CC Code: " << std::hex << cond;
        break;
    }

    return false;
}

// TODO: Get rid of ALL Exception handling
uint8_t Cpu::get_reg(int index)
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
    default:
        throw std::invalid_argument("Invalid index: " + index);
        break;
    }
}

uint8_t& Cpu::get_reg_ref(int index)
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
    default:
        throw std::invalid_argument("Invalid index: " + index);
        break;
    }
}

uint16_t Cpu::get_reg16(int index)
{
    switch (index)
    {
    case 0:
        return BC.r16;
    case 1:
        return DE.r16;
    case 2:
        return HL.r16;
    case 3:
        return SP;
    default:
        throw std::invalid_argument("Invalid index: " + index);
        break;
    }
}

uint16_t& Cpu::get_reg16_ref(int index)
{
    switch (index)
    {
    case 0:
        return BC.r16;
    case 1:
        return DE.r16;
    case 2:
        return HL.r16;
    case 3:
        return SP;
    default:
        throw std::invalid_argument("Invalid index: " + index);
        break;
    }
}

uint32_t Cpu::execute_instruction()
{    
    check_interrupts();

    if (is_halted) return 1;

    IR = mem.read_byte(PC++);

    ticks = 4;

    switch (IR)
    {
    // NOP
    // 4 T-cycles, 1 byte
    case 0x00: 
        break;

    // LD r16, n16
    // 0b00xx0001 + LSB(nn) + MSB(nn)
    // 12 T-cycles, 3 bytes
    case 0x01:
    case 0x11:
    case 0x21:
    case 0x31: 
    {
        int i = IR >> 4;
        ld_r16_n16(get_reg16_ref(i));

        ticks = 12;
        break;
    }

    // LD [r16], A
    // 0b00xx0010
    // 8 T-cycles
    case 0x02:
    case 0x12: 
    {
        int i = IR >> 4;
        mem.write_byte(A, get_reg16(i));

        ticks = 8;
        break;
    } 
    
    // INC r16
    // 0b00xx0011
    // 8 T-cycles
    case 0x03:
    case 0x13:
    case 0x23:
    case 0x33:
    {
        int i = IR >> 4;
        uint16_t& reg = get_reg16_ref(i);
        ++reg;

        ticks = 8;
        break;
    }

    // INC r8
    // 0b00xxx100
    // 4 T-cycles
    case 0x04:
    case 0x0C:
    case 0x14:
    case 0x1C:
    case 0x24:
    case 0x2C:
    case 0x3C:
    {
        int i = IR >> 3;
        inc_r8(get_reg_ref(i));
        break;
    }

    // DEC r8
    // 0b00xxx101
    // 4 T-cycles
    case 0x05:
    case 0x0D:
    case 0x15:
    case 0x1D:
    case 0x25:
    case 0x2D:
    case 0x3D:
    {
        int i = IR >> 3;
        dec_r8(get_reg_ref(i));
        break;
    }

    // LD r8, n8
    // 0b00xxx110 + nn
    // 8 T-cycles
    case 0x06:
    case 0x0E:
    case 0x16:
    case 0x1E:
    case 0x26:
    case 0x2E:
    case 0x3E:
    {
        int i = IR >> 3;
        uint8_t& r8 = get_reg_ref(i);
        r8 = mem.read_byte(PC++);

        ticks = 8;
        break;
    }
    
    // RLCA
    // 0x07
    // 4 T-cycles
    case 0x07:
        rlc_r8(A);
        set_flag(Flags::Zero, false);
        break;

    // LD [a16], SP
    // Copy SP & $FF at address n16 and SP >> 8 at address n16 + 1.
    // 0x08 + LSB(nn) + MSB(nn)
    // 20 T-cycles
    case 0x08:
    {
        uint16_t n16 = read_next16();

        mem.write_byte(SP & 0xFF, n16);
        mem.write_byte(SP >> 8, n16 + 1);

        ticks = 20;
        break;
    }

    // ADD HL, r16
    // 0b00xx1001
    // 8 T-cycles
    case 0x09:
    case 0x19:
    case 0x29:
    case 0x39:
    {
        int i = IR >> 4;
        add_hl_r16(get_reg16_ref(i));

        ticks = 8;
        break;
    }

    // LD A, [r16]
    // 0b00xx1010
    // 8 T-cycles
    case 0x0A:
    case 0x1A:
    {
        int i = IR >> 4;
        A = mem.read_byte(get_reg16(i));

        ticks = 8;
        break;
    }

    // DEC r16
    // 0b00xx1011
    // 8 T-cycles
    case 0x0B:
    case 0x1B:
    case 0x2B:
    case 0x3B:
    {
        int i = IR >> 4;
        uint16_t& reg = get_reg16_ref(i);
        --reg;

        ticks = 8;
        break;
    }

    // RRCA
    // 4 T-cycles
    case 0x0F:
        rrc_r8(A);
        set_flag(Flags::Zero, false);
        break;
    
    // STOP n8
    // 0x10 + n8
    // 4 T-cycles
    case 0x10:
        // is_halted = true;
        break;

    
    // RLA
    // 4 T-cycles
    case 0x17:
        rl_r8(A);
        set_flag(Flags::Zero, false);
        break;

    // JR e8
    // 12 T-cycles
    case 0x18:
    {
        uint8_t byte = mem.read_byte(PC++);
        PC += static_cast<int8_t>(byte);

        ticks = 12;
        break;
    }

    // RRA
    // 4 T-cycles
    case 0x1F:
        rr_r8(A);
        set_flag(Flags::Zero, false);
        break;
    
    // JR cc, e8
    // 0b001xx000
    // True/false -> 12/8 T-cycles
    case 0x20:
    case 0x30:
    case 0x28:
    case 0x38:
    {
        ticks = 8;

        uint8_t cc = (IR >> 3) & 0x03;
        uint8_t byte = mem.read_byte(PC++);
        if (check_condition_code(cc))
        {
            PC += static_cast<int8_t>(byte);
            ticks += 4;
        }
        break;
    }

    // LD [HL+], A
    // 8 T-cycles
    case 0x22:
        mem.write_byte(A, HL.r16);
        ++HL.r16;

        ticks = 8;
        break;

    // DAA
    // 4 T-cycles
    case 0x27:
        daa();
        break;

    // LD A, [HL+]
    // 8 T-cycles
    case 0x2A:
        A = mem.read_byte(HL.r16);
        ++HL.r16;

        ticks = 8;
        break;
    
    // CPL
    // 4 T-cycles
    case 0x2F:
        A = ~A;

        set_flag(Flags::Subtraction, true);
        set_flag(Flags::HalfCarry, true);
        break;
    
    // LD [HL-], A
    // 8 T-cycles
    case 0x32:
        mem.write_byte(A, HL.r16);
        --HL.r16;

        ticks = 8;
        break;
    
    // INC [HL]
    // 12 T-cycles
    case 0x34:
        inc_r8(mem.read_byte_ref(HL.r16));

        ticks = 12;
        break;
    
    // DEC [HL]
    // 12 T-cycles
    case 0x35:
        dec_r8(mem.read_byte_ref(HL.r16));

        ticks = 12;
        break;

    // LD [HL], n8
    // 12 T-cycles
    case 0x36:
    {
        uint8_t n8 = mem.read_byte(PC++);
        mem.write_byte(n8, HL.r16);

        ticks = 12;
        break;
    }

    // SCF
    // 4 T-cycles
    case 0x37:
        set_flag(Flags::HalfCarry, false);
        set_flag(Flags::Subtraction, false);
        set_flag(Flags::Carry, true);
        break;
    
    
    // LD A, [HL-]
    // 8 T-cycles
    case 0x3A:
        A = mem.read_byte(HL.r16);
        --HL.r16;

        ticks = 8;
        break;
    
    // CCF
    // 4 T-cycles
    case 0x3F:
        set_flag(Flags::Subtraction, false);
        set_flag(Flags::HalfCarry, false);
        set_flag(Flags::Carry, !check_flag(Flags::Carry)); 
        break;
    
    // LD B, r8
    // 0b01xxxyyy - xxx = src, yyy = dst
    // 4 T-cycles
    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
    case 0x44:
    case 0x45:
    case 0x47:
        BC.high = get_reg(IR & LD_SRC_BITMASK);
        break;

    // LD C, r8
    // 4 T-cycles
    case 0x48:    
    case 0x49:
    case 0x4A:
    case 0x4B:
    case 0x4C:
    case 0x4D:
    case 0x4F:
        BC.low = get_reg(IR & LD_SRC_BITMASK);
        break;

    // LD D, r8
    // 4 T-cycles
    case 0x50:
    case 0x51:
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x55:
    case 0x57:
        DE.high = get_reg(IR & LD_SRC_BITMASK);
        break;
    
    // LD E, r8
    // 4 T-cycles
    case 0x58:
    case 0x59:
    case 0x5A:
    case 0x5B:
    case 0x5C:
    case 0x5D:
    case 0x5F:
        DE.low = get_reg(IR & LD_SRC_BITMASK);
        break;

    // LD H, r8
    // 4 T-cycles
    case 0x60:
    case 0x61:
    case 0x62:
    case 0x63:
    case 0x64:
    case 0x65:
    case 0x67:
        HL.high = get_reg(IR & LD_SRC_BITMASK);
        break;

    // LD L, r8
    // 4 T-cycles
    case 0x68:
    case 0x69:
    case 0x6A:
    case 0x6B:
    case 0x6C:
    case 0x6D:
    case 0x6F:
        HL.low = get_reg(IR & LD_SRC_BITMASK);
        break;

    // LD [HL], r8
    // 8 T-cycles
    case 0x70:
    case 0x71:
    case 0x72:
    case 0x73:
    case 0x74:
    case 0x75:
    case 0x77:
        mem.write_byte(get_reg(IR & LD_SRC_BITMASK), HL.r16);

        ticks = 8;
        break;
    
    // LD r8, [HL]
    // 0b01xxx110
    // 8 T-cycles
    case 0x46:
    case 0x4E:
    case 0x56:
    case 0x5E:
    case 0x66:
    case 0x6E:
    case 0x7E:
    {
        int i = (IR & LD_DST_BITMASK) >> 3;
        uint8_t& reg = get_reg_ref(i);
        reg = mem.read_byte(HL.r16);

        ticks = 8;
        break;
    }
    
    // HALT
    // 4 T-cycles
    case 0x76:
        is_halted = true;
        break;

    // LD A, r8
    // 4 T-cycles
    case 0x78:
    case 0x79:
    case 0x7A:
    case 0x7B:
    case 0x7C:
    case 0x7D:
    case 0x7F:
        AF.high = get_reg(IR & LD_SRC_BITMASK);
        break;

    // ADD A, r8
    // 0b10000xxx
    // 4 T-cycles
    case 0x80:
    case 0x81:
    case 0x82:
    case 0x83:
    case 0x84:
    case 0x85:
    case 0x87:
    {
        int i = IR & ARITHMETIC_OP_BITMASK;
        add_a(get_reg(i));
        break;
    }

    // ADD A, [HL]
    // 8 T-cycles  
    case 0x86:
        add_a(mem.read_byte(HL.r16));

        ticks = 8;
        break;

    // ADC A, r8
    // 0b10001xxx
    // 4 T-cycles
    case 0x88:
    case 0x89:
    case 0x8A:
    case 0x8B:
    case 0x8C:
    case 0x8D:
    case 0x8F:
    {
        int i = IR & ARITHMETIC_OP_BITMASK;
        adc_a(get_reg(i));
        break;
    }

    // ADC A, HL
    // 8 T-cycles
    case 0x8E:
        adc_a(mem.read_byte_ref(HL.r16));

        ticks = 8;
        break;

    // SUB A, r8
    // 0b10010xxx
    // 4 T-cycles
    case 0x90:
    case 0x91:
    case 0x92:
    case 0x93:
    case 0x94:
    case 0x95:
    case 0x97:
    {
        int i = IR & ARITHMETIC_OP_BITMASK;
        sub_a(get_reg(i));
        break;
    }

    // SUB A, HL
    // 8 T-cycles
    case 0x96:
        sub_a(mem.read_byte(HL.r16));

        ticks = 8;
        break;

    // SBC A, r8
    // 0b10011xxx
    // 4 T-cycles
    case 0x98:
    case 0x99:
    case 0x9A:
    case 0x9B:
    case 0x9C:
    case 0x9D:
    case 0x9F:
    {
        int i = IR & ARITHMETIC_OP_BITMASK;
        sbc_a(get_reg(i));
        break;
    }

    // SBC A, HL
    // 8 T-cycles
    case 0x9E:
        sbc_a(mem.read_byte(HL.r16));

        ticks = 8;
        break;

    // AND A, r8
    // 0b10100xxx
    // 4 T-cycles
    case 0xA0:
    case 0xA1:
    case 0xA2:
    case 0xA3:
    case 0xA4:
    case 0xA5:
    case 0xA7:
    {
        int i = IR & ARITHMETIC_OP_BITMASK;
        and_a(get_reg(i));
        break;
    }

    // AND A, HL
    // 8 T-cycles
    case 0xA6:
        and_a(mem.read_byte(HL.r16));

        ticks = 8;
        break;

    // XOR A, r8
    // 0b10101xxx
    // 4 T-cycles
    case 0xA8:
    case 0xA9:
    case 0xAA:
    case 0xAB:
    case 0xAC:
    case 0xAD:
    case 0xAF:
    {
        int i = IR & ARITHMETIC_OP_BITMASK;
        xor_a(get_reg(i));
        break;
    }

    // XOR A, HL
    // 8 T-cycles
    case 0xAE:
        xor_a(mem.read_byte(HL.r16));

        ticks = 8;
        break;

    // OR A, r8
    // 0b10110xxx
    // 4 T-cycles
    case 0xB0:
    case 0xB1:
    case 0xB2:
    case 0xB3:
    case 0xB4:
    case 0xB5:
    case 0xB7:
    {
        int i = IR & ARITHMETIC_OP_BITMASK;
        or_a(get_reg(i));
        break;
    }

    // OR A, HL
    // 8 T-cycles
    case 0xB6:
        or_a(mem.read_byte(HL.r16));

        ticks = 8;
        break;

    // CP A, r8
    // 0b10111xxx
    // 4 T-cycles
    case 0xB8:
    case 0xB9:
    case 0xBA:
    case 0xBB:
    case 0xBC:
    case 0xBD:
    case 0xBF:
    {
        int i = IR & ARITHMETIC_OP_BITMASK;
        cp_a(get_reg(i));
        break;
    }

    // CP A, HL
    // 8 T-cycles
    case 0xBE:
        cp_a(mem.read_byte(HL.r16));

        ticks = 8;
        break;

    // 0b110xx000
    // RET CC
    // 1 byte
    // True/False -> 20/8 T-cycles
    case 0xC0:
    case 0xC8:
    case 0xD0:
    case 0xD8:
    {  
        ticks = 8;

        uint8_t cc = (IR >> 3) & 0x03;
        if (check_condition_code(cc))
        {
            pop_r16(PC);
            ticks += 12;
        }
        break;
    }

    // POP r16
    // 12 T-cycles
    // 0b11xx0001
    case 0xC1:
    case 0xD1:
    case 0xE1:
        pop_r16(get_reg16_ref((IR >> 4) & 0x03));

        ticks = 12;
        break;
    // POP AF
    case 0xF1:
        pop_r16(AF.r16);
        F &= HIGH_NIBBLE_MASK;

        ticks = 12;
        break;

    // JP CC a16
    // 0b110xx010
    // True/False -> 16/12 T-Cycles
    case 0xC2:
    case 0xCA:
    case 0xD2:
    case 0xDA:
    {
        ticks = 12;

        uint8_t cc = (IR >> 3) & 0x03;
        uint16_t addr = read_next16();
        if (check_condition_code(cc))
        {
            PC = addr;
            ticks += 4;
        }
        break;
    }
    
    // JP a16
    // 16 T-cycles
    case 0xC3:
        PC = read_next16();
        
        ticks = 16;
        break;

    // CALL CC a16
    // True/False -> 24/12 T-Cycles
    // 3 bytes
    // 0b110xx100 + LSB + MSB
    case 0xC4:
    case 0xCC:
    case 0xD4:
    case 0xDC:
    {
        ticks = 12;

        uint8_t cc = (IR >> 3) & 0x03;
        uint16_t imm16 = read_next16();

        if (check_condition_code(cc))
        {
            call_n16(imm16);

            ticks += 12;
        }
        break;
    }

    // PUSH r16
    // 16 T-Cycles
    // 1 byte
    // 0b11xx0101
    case 0xC5:
    case 0xD5:
    case 0xE5:
        push_r16(get_reg16((IR >> 4) & 0x03));

        ticks = 16;
        break;
    // POP AF
    case 0xF5:
        push_r16(AF.r16);

        ticks = 16;
        break;

    // ADD A, n8
    // 8 T-cycles
    case 0xC6:
        add_a(mem.read_byte(PC++));

        ticks = 8;
        break;
    // SUB A, n8
    case 0xD6:
        sub_a(mem.read_byte(PC++));

        ticks = 8;
        break;
    // AND A, n8
    case 0xE6:
        and_a(mem.read_byte(PC++));

        ticks = 8;
        break;
    // OR A, n8
    case 0xF6:
        or_a(mem.read_byte(PC++));

        ticks = 8;
        break;

    // RST vec
    // 16 T-cycles
    // 1 byte
    case 0xC7:
    case 0xCF:
    case 0xD7:
    case 0xDF:
    case 0xE7:
    case 0xEF:
    case 0xF7:
    case 0xFF:
        call_n16(static_cast<uint16_t>(IR - 0xC7));

        ticks = 16;
        break;

    // RET
    // 16 T-cycles
    case 0xC9:
        pop_r16(PC);

        ticks = 16;
        break;
    
    // CB-Prefixed Opocodes
    case 0xCB:
        cb_execute();

        ticks = 8;
        break;

    // CALL a16
    // 24 T-Cycles
    // 3 bytes
    case 0xCD:
    {
        uint16_t imm16 = read_next16();
        call_n16(imm16);
        
        ticks = 24;
        break;
    }

    // ADC A, n8
    // 8 T-cycles
    // 2 bytes
    case 0xCE:
        adc_a(mem.read_byte(PC++));

        ticks = 8;
        break;
    // SBC A, n8
    case 0xDE:
        sbc_a(mem.read_byte(PC++));

        ticks = 8;
        break;
    // XOR A, n8
    case 0xEE:
        xor_a(mem.read_byte(PC++));

        ticks = 8;
        break;
    // CP A, n8
    case 0xFE:
        cp_a(mem.read_byte(PC++));
        
        ticks = 8;
        break;

    // RETI
    // 16 T-Cycles
    // 1 byte
    case 0xD9:
        pop_r16(PC);
        IME = true;

        ticks = 16;
        break;

    // LDH [a8], A
    // 12 T-cycles
    // 2 bytes
    case 0xE0:
    {
        uint8_t imm8 = mem.read_byte(PC++);
        mem.write_byte(A, IO_REGISTERS_START + imm8);

        ticks = 12;
        break;
    }

    // LDH [C], A
    // 8 T-cycles
    // 1 byte
    case 0xE2:
        mem.write_byte(A, IO_REGISTERS_START + BC.low);

        ticks = 8;
        break;

    // ADD SP, e8
    // 16 T-cycles
    // 2 bytes
    case 0xE8:
    {
        int8_t imm8 = static_cast<int8_t>(mem.read_byte(PC++));
        
        set_flag(Flags::Carry, check_carry(SP, imm8));
        set_flag(Flags::HalfCarry, check_half_carry(SP, imm8));

        SP += imm8;

        set_flag(Flags::Zero, false);
        set_flag(Flags::Subtraction, false);

        ticks = 16;
        break;
    }

    // JP HL
    // 4 T-cycles
    case 0xE9:
        PC = HL.r16;
        break;

    // LD [a16], A
    // 16 T-cycles
    // 3 bytes
    case 0xEA:
    {
        uint16_t imm16 = read_next16();
        mem.write_byte(A, imm16);

        ticks = 16;
        break;
    }

    // LDH A, [a8]
    // 12 T-cycles
    // 2 bytes
    case 0xF0:
    {
        uint8_t imm8 = mem.read_byte(PC++);
        A = mem.read_byte(IO_REGISTERS_START + imm8);

        ticks = 12;
        break;
    }

    // LDH A, [C]
    // 8 T-cycles
    // 1 byte
    case 0xF2:
        A = mem.read_byte(IO_REGISTERS_START + BC.low);

        ticks = 8;
        break;

    // DI
    // 4 T-cycles
    case 0xF3:
        IME = false;
        break;

    // LD HL, SP + e8
    // 12 T-cycles
    // 2 bytes
    case 0xF8:
    {
        int8_t imm8 = static_cast<int8_t>(mem.read_byte(PC++));

        set_flag(Flags::Carry, check_carry(SP, imm8)); 
        set_flag(Flags::HalfCarry, check_half_carry(SP, imm8)); 

        HL.r16 = SP + imm8;

        set_flag(Flags::Zero, false);
        set_flag(Flags::Subtraction, false);

        ticks = 12;
        break;
    }

    // LD SP, HL
    // 8 T-cycles
    // 1 bytes
    case 0xF9:
        SP = HL.r16;

        ticks = 8;
        break;

    // LD A, [a16]
    // 16 T-cycles
    // 3 bytes
    case 0xFA:
    {
        uint16_t imm16 = read_next16();
        A = mem.read_byte(imm16);

        ticks = 16;
        break;
    }

    // EI
    // Note: The effect of ei is delayed by one instruction. 
    // This means that ei followed immediately by di does not 
    // allow any interrupts between them.
    // TODO: Implement the above
    // 4 T-cycles
    case 0xFB:
        IME = true;
        break;

    default:
        std::cout << "Invalid Opcode: " << std::hex << +IR << '\n';
        break;
    }

    return ticks;
}

void Cpu::cb_execute()
{
    IR = mem.read_byte(PC++);

    uint8_t u3 = (IR & CB_U3_BITMASK) >> 3;
    uint8_t op = IR & CB_OP_BITMASK;
    
    switch(IR)
    {
    // RLC r8
    // 0b00000xxx
    // 8 T-cycles
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x07: // Potential Issue
        rlc_r8(get_reg_ref(op));
        break;

    // RLC [HL]
    // 16 T-cycles
    case 0x06:
        rlc_r8(mem.read_byte_ref(HL.r16));

        ticks = 16;
        break;

    // RRC r8
    // 0b00001xxx
    // 8 T-cycles
    case 0x08:
    case 0x09:
    case 0x0A:
    case 0x0B:
    case 0x0C:
    case 0x0D:
    case 0x0F: 
        rrc_r8(get_reg_ref(op));
        break;

    // RRC [HL]
    // 16 T-cycles
    case 0x0E:
        rrc_r8(mem.read_byte_ref(HL.r16));

        ticks = 16;
        break;

    // RL r8
    // 0b00010xxx
    // 8 T-cycles
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x14:
    case 0x15:
    case 0x17:
        rl_r8(get_reg_ref(op));
        break;

    // RL [HL]
    // 16 T-cycles
    case 0x16:
        rl_r8(mem.read_byte_ref(HL.r16));

        ticks = 16;
        break;

    // RR r8
    // 0b00011xxx
    // 8 T-cycles
    case 0x18:
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1F: 
        rr_r8(get_reg_ref(op));
        break;

    // RR [HL]
    // 16 T-cycles
    case 0x1E:
        rr_r8(mem.read_byte_ref(HL.r16));

        ticks = 16;
        break;

    // SLA r8
    // 0b00100xxx
    // 8 T-cycles
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x27: 
        sla_r8(get_reg_ref(op));
        break;

    // SLA [HL]
    // 16 T-cycles
    case 0x26:
        sla_r8(mem.read_byte_ref(HL.r16));

        ticks = 16;
        break;

    // SRA r8
    // 0b00101xxx
    // 8 T-cycles
    case 0x28:
    case 0x29:
    case 0x2A:
    case 0x2B:
    case 0x2C:
    case 0x2D:
    case 0x2F: 
        sra_r8(get_reg_ref(op));
        break;

    // SRA [HL]
    // 16 T-cycles
    case 0x2E:
        sra_r8(mem.read_byte_ref(HL.r16));

        ticks = 16;
        break;

    // SWAP r8
    // 0b00110xxx
    // 8 T-cycles
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x37:
        swap_r8(get_reg_ref(op));
        break;

    // SWAP [HL]
    // 16 T-cycles
    case 0x36:
        swap_r8(mem.read_byte_ref(HL.r16));

        ticks = 16;
        break;

    // SRL r8
    // 0b00111xxx
    // 8 T-cycles
    case 0x38:
    case 0x39:
    case 0x3A:
    case 0x3B:
    case 0x3C:
    case 0x3D:
    case 0x3F: 
        srl_r8(get_reg_ref(op));
        break;

    // SRL [HL]
    // 16 T-cycles
    case 0x3E:
        srl_r8(mem.read_byte_ref(HL.r16));

        ticks = 16;
        break;
    
    // Bit u3, r8
    // 01yyyxxx
    // 8 T-cycles
    case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x47: 
    case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4F: 
    case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x57: 
    case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5F: 
    case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x67: 
    case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6F: 
    case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x77: 
    case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7F: 
        bit_u3_r8(get_reg_ref(op), u3);
        break;

    // Bit u3, r8
    // 12 T-cycles
    case 0x46:
    case 0x4E:
    case 0x56:
    case 0x5E:
    case 0x66:
    case 0x6E:
    case 0x76:
    case 0x7E:
        bit_u3_r8(mem.read_byte_ref(HL.r16), u3);

        ticks = 12;
        break;

    // RES u3, r8
    // 10yyyxxx
    // 8 T-cycles
    case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x87: 
    case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8F: 
    case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x97: 
    case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9F: 
    case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA7: 
    case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAF: 
    case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB7: 
    case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBF:
        res_u3_r8(get_reg_ref(op), u3);
        break;

    // RES u3, [HL]
    // 16 T-cycles
    case 0x86:
    case 0x8E:
    case 0x96:
    case 0x9E:
    case 0xA6:
    case 0xAE:
    case 0xB6:
    case 0xBE:
        res_u3_r8(mem.read_byte_ref(HL.r16), u3);

        ticks = 16;
        break;
    
    // SET u3, r8
    // 8 T-cycles
    case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5: case 0xC7: 
    case 0xC8: case 0xC9: case 0xCA: case 0xCB: case 0xCC: case 0xCD: case 0xCF: 
    case 0xD0: case 0xD1: case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD7: 
    case 0xD8: case 0xD9: case 0xDA: case 0xDB: case 0xDC: case 0xDD: case 0xDF: 
    case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5: case 0xE7: 
    case 0xE8: case 0xE9: case 0xEA: case 0xEB: case 0xEC: case 0xED: case 0xEF: 
    case 0xF0: case 0xF1: case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF7: 
    case 0xF8: case 0xF9: case 0xFA: case 0xFB: case 0xFC: case 0xFD: case 0xFF: 
        set_u3_r8(get_reg_ref(op), u3);
        break;

    // SET u3, [HL]
    // 16 T-cycles
    case 0xC6:
    case 0xCE:
    case 0xD6:
    case 0xDE:
    case 0xE6:
    case 0xEE:
    case 0xF6:
    case 0xFE:
        set_u3_r8(mem.read_byte_ref(HL.r16), u3);

        ticks = 16;
        break;

    default:
        break;
    }
}



/* CPU Instructions */

void Cpu::invalid_opcode() const
{
    std::cout << "Invalid Opcode: " << std::hex << +IR << '\n';
}


// 0b10001xxx ~
// Good
void Cpu::adc_a(uint8_t reg8)
{
    uint8_t carry = static_cast<uint8_t>(check_flag(Flags::Carry));

    set_flag(Flags::Carry, check_carry(A, reg8, carry));
    set_flag(Flags::HalfCarry, check_half_carry(A, reg8, carry));

    A += reg8 + carry;

    set_flag(Flags::Zero, A == 0);
    set_flag(Flags::Subtraction, false);
}

// Good
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
    set_flag(Flags::Carry, check_borrow(A, reg8));
    set_flag(Flags::HalfCarry, check_half_borrow(A, reg8));

    A -= reg8;

    set_flag(Flags::Zero, A == 0);
    set_flag(Flags::Subtraction, true);
}


void Cpu::sbc_a(uint8_t reg8)
{
    uint8_t carry = static_cast<uint8_t>(check_flag(Flags::Carry));

    set_flag(Flags::Carry, check_borrow(A, reg8, carry));
    set_flag(Flags::HalfCarry, check_half_borrow(A, reg8, carry));

    A -= carry;
    A -= reg8;

    set_flag(Flags::Zero, A == 0);
    set_flag(Flags::Subtraction, true);
}

void Cpu::cp_a(uint8_t reg8) 
{
    set_flag(Flags::Carry, check_borrow(A, reg8));
    set_flag(Flags::HalfCarry, check_half_borrow(A, reg8));

    uint8_t res = A - reg8;

    set_flag(Flags::Zero, res == 0);
    set_flag(Flags::Subtraction, true);
}


//  0b00xxx100 ~ 
void Cpu::inc_r8(uint8_t& reg8)
{
    set_flag(Flags::HalfCarry, check_half_carry(reg8, 1));

    ++reg8;

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
    A &= reg8;

    set_flag(Flags::Zero, A == 0);
    set_flag(Flags::Subtraction, false);
    set_flag(Flags::HalfCarry, true);
    set_flag(Flags::Carry, false);
}


void Cpu::or_a(uint8_t reg8)
{
    A |= reg8;

    set_flag(Flags::Zero, A == 0);
    set_flag(Flags::Subtraction, false);
    set_flag(Flags::HalfCarry, false);
    set_flag(Flags::Carry, false);
}

void Cpu::xor_a(uint8_t reg8)
{
    A ^= reg8;

    set_flag(Flags::Zero, A == 0);
    set_flag(Flags::Subtraction, false);
    set_flag(Flags::HalfCarry, false);
    set_flag(Flags::Carry, false);
}


/* 16-bit Arithmetic/Logic Operations */

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

/* Stack Operations */
void Cpu::push_r16(uint16_t reg)
{
    // DEC SP
    // LD [SP], HIGH(r16)  ; B, D or H
    // DEC SP
    // LD [SP], LOW(r16)   ; C, E or L

    mem.write_byte(reg >> 8, --SP);
    mem.write_byte(reg & 0xFF, --SP);
}

void Cpu::pop_r16(uint16_t& reg)
{
    // LD LOW(r16), [SP]   ; C, E or L
    // INC SP
    // LD HIGH(r16), [SP]  ; B, D or H
    // INC SP
    uint8_t low = mem.read_byte(SP++);
    uint8_t high = mem.read_byte(SP++);

    reg = (high << 8) | low;
}


inline void Cpu::call_n16(uint16_t addr) 
{
    push_r16(PC);

    PC = addr;
}

/* Bit operations */
// CB Prefix + 0b00110xxx
void Cpu::swap_r8(uint8_t& reg8)
{
    int i = IR & CB_OP_BITMASK;

    uint8_t low_nibble = reg8 & LOW_NIBBLE_MASK;
    uint8_t hi_nibble = (reg8 & HIGH_NIBBLE_MASK) >> 4;

    reg8 = (low_nibble << 4) | hi_nibble;

    set_flag(Flags::Zero, reg8 == 0);
    set_flag(Flags::Subtraction, false);
    set_flag(Flags::HalfCarry, false);
    set_flag(Flags::Carry, false);
}

// CB-Prefix + 0b01yyyxxx - xxx = operand, yyy = bit index
void Cpu::bit_u3_r8(uint8_t& reg8, uint8_t bit_index)
{
    uint8_t bit_to_check = reg8 & (0x1 << bit_index);

    set_flag(Flags::Zero, bit_to_check == 0);
    set_flag(Flags::Subtraction, false);
    set_flag(Flags::HalfCarry, true);
}

// CB-Prefix + 0b10yyyxxx - xxx = operand, yyy = bit index
void Cpu::res_u3_r8(uint8_t& reg8, uint8_t bit_index)
{
    reg8 &= ~(0x1 << bit_index);
}

// CB Prefix + 0b11yyyxxx - xxx = operand, yyy = bit index
void Cpu::set_u3_r8(uint8_t& reg8, uint8_t bit_index)
{
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
// Good
void Cpu::sra_r8(uint8_t& reg8)
{
    uint8_t msb = reg8 & MSB_BITMASK;
    uint8_t lsb = reg8 & LSB_BITMASK;

    reg8 >>= 1;
    reg8 |= msb;

    set_flag(Flags::Carry, lsb == LSB_BITMASK);
    set_flag(Flags::Zero, reg8 == 0);
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

    uint8_t lsb = reg8 & LSB_BITMASK;

    reg8 >>= 1;

    set_flag(Flags::Carry, lsb == LSB_BITMASK);
    set_flag(Flags::Zero, reg8 == 0);
} // Good

// CB Prefix + 0b00100xxx - xxx = register
// -> 2 bytes
// -> 2 cycles
// Good
void Cpu::sla_r8(uint8_t& reg8)
{
    set_flag(Flags::Subtraction, false);
    set_flag(Flags::HalfCarry, false);

    uint8_t msb = reg8 & MSB_BITMASK;

    reg8 <<= 1;

    set_flag(Flags::Carry, msb == MSB_BITMASK);
    set_flag(Flags::Zero, reg8 == 0);
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

    set_flag(Flags::Carry, lsb == LSB_BITMASK);
    set_flag(Flags::Zero, reg8 == 0);
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

    set_flag(Flags::Carry, msb == MSB_BITMASK);
    set_flag(Flags::Zero, reg8 == 0);
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


    uint8_t msb = reg8 & MSB_BITMASK;
    reg8 <<= 1;

    reg8 |= (msb >> 7);

    set_flag(Flags::Carry, msb == MSB_BITMASK);
    set_flag(Flags::Zero, reg8 == 0);
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

    reg8 |= (lsb << 7);

    set_flag(Flags::Carry, lsb == LSB_BITMASK);
    set_flag(Flags::Zero, reg8 == 0);
}

// 0b00100111/0x27
// 1 cycle
// 1 byte
inline void Cpu::daa()
{
    uint8_t offset = 0;
    bool is_subtraction = check_flag(Flags::Subtraction);

    if ((!is_subtraction && ((A & LOW_NIBBLE_MASK) > 0x09)) || check_flag(Flags::HalfCarry))
        offset |= 0x06;

    if ((!is_subtraction && A > 0x99) || check_flag(Flags::Carry))
    {
        offset |= 0x60;
        set_flag(Flags::Carry, true); 
    }
    else set_flag(Flags::Carry, false);

    A = (is_subtraction) ? A - offset : A + offset;

    set_flag(Flags::Zero, A == 0);
    set_flag(Flags::HalfCarry, false);
}

void Cpu::check_interrupts()
{
    uint8_t interrupt_check = mem.read_byte(INTERRUPT_ENABLE) & mem.read_byte(INTERRUPT_FLAG) & HIGH_NIBBLE_MASK;

    if (interrupt_check != 0)
    {
        is_halted = false;
        
        if (IME) handle_interrupt();
        else 
        {
            // TODO: Implement HALT Bug
        }
    }
}

// 20 T-cycles
// 8 T-cycles = 2 NOPs
// 8 T-cycles = push PC onto stack
// 4 T-cycles = set PC to Interrupt Address
void Cpu::handle_interrupt()
{
    std::cout << "Handler" << std::endl;
    IME = false;

    uint8_t interrupt_flag = mem.read_byte(INTERRUPT_FLAG);
    uint8_t interrupt = interrupt_flag & mem.read_byte(INTERRUPT_ENABLE);

    if ((interrupt & static_cast<uint8_t>(Interrupts::VBlank)) != 0)
    {
        mem.write_byte(interrupt_flag & ~static_cast<uint8_t>(Interrupts::VBlank), INTERRUPT_FLAG);
        call_n16(VBLANK_INTERRUPT_START);
    }
    else if ((interrupt & static_cast<uint8_t>(Interrupts::LCD)) != 0)
    {
        mem.write_byte(interrupt_flag & ~static_cast<uint8_t>(Interrupts::LCD), INTERRUPT_FLAG);
        call_n16(SERIAL_INTERRUPT_START);
    }

    else if ((interrupt & static_cast<uint8_t>(Interrupts::Timer)) != 0)
    {
        mem.write_byte(interrupt_flag & ~static_cast<uint8_t>(Interrupts::Timer), INTERRUPT_FLAG);
        call_n16(TIMER_INTERRUPT_STARRT);
    }

    else if ((interrupt & static_cast<uint8_t>(Interrupts::Serial)) != 0)
    {
        mem.write_byte(interrupt_flag & ~static_cast<uint8_t>(Interrupts::Serial), INTERRUPT_FLAG);
        call_n16(SERIAL_INTERRUPT_START);
    }

    else if ((interrupt & static_cast<uint8_t>(Interrupts::JoyPad)) != 0)
    {
        mem.write_byte(interrupt_flag & ~static_cast<uint8_t>(Interrupts::VBlank), INTERRUPT_FLAG);
        call_n16(JOYPAD_INTERRUPT_START);
    }

    ticks = 20;
}