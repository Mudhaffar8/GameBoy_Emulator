# GameBoy Emulator (WIP)

This is a work-in-progress GameBoy emulator written in C++. The primary goal of this project is to create a fast, memory-efficient emulator that can run smoothly even on older hardware.

## Current Features
- Full implementation of all 501 GameBoy opcodes, with all instructions (excpet for EI) passing [SingleStepTest's SM83 Test Suite](https://github.com/SingleStepTests/sm83). For more info, check out sm83-tests branch.
- Interrupt handling implemented and tested.
- Cycle counting for all instructions complete
- (Almost) all I/O registers memory-mapped.
- Basic implementation of the Timer (still needs to be tested).
- Most PPU features supported:
  - Per-scanline rendering
  - Background, window, and sprite layers
  - X and Y scrolling behaviors for all 3
  - All sprite attributes (X & Y flipping, sprite vs. bg priority)
  - BP, OBP0, and OBP1 palettes
 
## Planned Updates
- Add more PPU features:
  - PPU Mode switching (OAM Scan, HBlank, VBlank, Drawing)
  - Run PPU Test ROMs
- Test and fix the timer.

This project is in active development, so stay tuned for updates.
