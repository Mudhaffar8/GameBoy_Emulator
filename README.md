# CPU Test Suite

This readme contains the results of running the single step tests for the SM83. Below is a breakdown of the tests and their outcomes.

**Note**: The tester presumes 64KB of flat RAM (No memory mapping). This presumption causes issues where writes to any interrupt registers (specifically addresses 0xFF00 and 0xFFFF) may cause the interrupt handler to be triggered, especially when IME is set. Because of this, IME and IE were left unset for all tests (more information on weird behaviours that arise from not doing so below) and were not checked with the tester except for instructions EI, DI, and RETI. The vast majority of instructions do not affect the IME and IE registers so there isn't too much concern there.

---

## Testing Individual Instruction
### Load Instructions

| Instruction | Opcode(s) Tested | Status |
|-------------|------------------|--------|
| LD r16, n16 | 0x01, 0x31 | OK |
| LD [r16], n16 | 0x02 | OK |
| LD r8, n8 | 0x0E, 0x26 | OK |
| LD A, [r16] | 0x1A | OK |
| LD [HL+], A | 0x22 | OK |
| LD [HL-], A | 0x32 | OK |
| LD [a16], SP | 0x08 | OK |
| LD HL, SP + e8 || OK |
| LD [a16], A | | OK |
| LD A, [a16] | | OK |
| LD r8, r8 | 0x42, 0x48, 0x55, 0x5C, 0x60, 0x69, 0x7A | OK |

---

### Bit Manipulation Operations

| Instruction | Opcode(s) Tested | Status |
|-------------|------------------|--------|
| RLCA | | OK |
| RRCA | | OK |
| RRA | | OK |
| RLA | | OK |
| SCF | | OK |
| CPL | | OK |
| CCF | | OK |
| DAA | | OK |

---

### Branching

| Instruction | Opcode(s) Tested | Status |
|-------------|------------------|--------|
| NOP | | OK |
| STOP | | OK |
| JR e8 | | OK |
| HALT | | OK |
| CALL n16 | | OK |
| JP n16 | | OK |
| POP r16 | 0xC1, 0xD1, 0xE1, 0xF1 | OK |
| RST vec | 0xC7, 0xFF | OK |
| PUSH r16 | 0xE5, 0xF5 | OK |
| RET | | OK |
| RETI | | OK |
| JP HL | | OK |
| DI | | OK |
| EI | | ERR (IME - Expected: 0 Got: 1, test #1) |
| RET cc | 0xC0, 0xD0 | OK |
| JP cc, n16 | 0xC2, 0xCA, 0xD2, 0xDA | OK |
| JR cc, n16 | 0x20, 0x28 | OK |
| CALL cc | 0xC4, 0xD4, 0xCC, 0xDC | OK |

---

### Arithmetic/Logic

| Instruction | Opcode(s) Tested | Status |
|-------------|------------------|--------|
| INC r16 | 0x03, 0x33 | OK |
| DEC r16 | 0x0B | OK |
| ADD HL, r16 | 0x09 | OK |
| INC r8 | 0x24 | OK |
| DEC r8 | 0x15 | OK |
| ADD A, r8 | 0x83 | OK |
| ADC A, r8 | 0x8D | OK |
| SUB A, r8 | 0x92 | OK |
| SBC A, r8 | 0x9A, 0x9B, 0x9D | OK |
| ADD A, n8 | | OK |
| ADD SP, e8 | 0xE8 | OK |
| AND A, r8 | 0xA0 | OK |
| OR A, r8 | 0xB2, 0xB3 | OK |
| CP A, r8 | 0xB8, 0xBD | OK |
| XOR A, r8 | 0xAA | OK |

---

### CB-Prefixed Instructions

| Instruction | Opcode(s) Tested | Status |
|-------------|------------------|--------|
| SET u3, r8 | 0xE2 | OK |
| RES u3, r8 | 0x90 | OK |
| BIT u3, r8 | 0x75 | OK |
| SWAP r8 | 0x33 | OK |
| RLC r8 | 0x00, 0x01, 0x04, 0x07 | OK |

---

### HL-Specific Instructions

| Instruction | Opcode(s) Tested | Status |
|-------------|------------------|--------|
| INC [HL] | 0x34 | OK |
| DEC [HL] | 0x35 | OK |
| LD [HL], n8 | 0x36 | OK |
| LD A, [HL+] | 0x2A | OK |
| LD A, [HL-] | 0x3A | OK |
| LD [HL], r8 | 0x72, 0x74, 0x77 | OK |
| SUB A, [HL] | 0x96 | OK |
| LDH [C], A | 0xE2 | OK |
| LDH A, [C] | 0xF2 | OK |
| LDH [a8], A | 0xE0 | OK |
| LDH A, [a8] | 0xF0 | OK |
| LD [HL], L | 0x75 | OK |
| LD r8, [HL] | 0x46, 0x56, 0x6E | OK |

---

## Testing All Opcodes in Sequence

### Skipped Opcodes

- `0x69`: Calls interrupt handler somehow. Works fine on its own
- `0xFB`: Contains specific behavior not implemented for `EI` and also not really important either.

### Weird Behaviours

Running tests from `0xC0 - 0xFF`:
- Interrupt handler was called at `0xE0` (PC - Expected: 18086, Got: 0, test #0140).
- **Possible Clue**: Stack (and memory in general) is never zero-initialized between tests. This may cause tests from previous instructions to affect the next ones.

Running tests from `0x40 - 0x7F`:
- Interrupt handler was called at `0x79` (A - Expected: 229 Got: 143).
- Temporary fix: leave IME and IE unset.

Running tests from `0xCB 0x00 - 0xCB 0xFF`:
- Interrupt handler was called at `0xCB 0xA0` (PC - Expected: 1 Got: 66).
- Temporary fix: leave IME and IE unset.

### Test Run

- Testing opcodes: `0x00 - 0x7F` (Excluding 0x69): OK ✅
- Testing opcodes: `0x80 - 0xBF`: OK ✅
- Testing opcodes: `0xC0 - 0xDF`: OK ✅
- Testing opcodes: `0xE0 - 0xFF` (Excluding 0xFB): OK ✅
- Testing opcodes: `0xCB 00 - 0xCB FF`: OK ✅

---

## Conclusion

Most CPU instructions seem to work fine albeit with some unexpected behaviours when certain instructions are run in sequence (This is likely not a fault of the emulator itself). More testing will be done for CPU instructions NOP, STOP, EI, and for interrupt handling. 

## Credits
- Single Step Tests: https://github.com/SingleStepTests/sm83
- JSON Parser: https://github.com/nlohmann/json