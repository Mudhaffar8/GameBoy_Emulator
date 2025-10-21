# GameBoy Emulator (WIP)

This is a work-in-progress GameBoy emulator written in C++, with plans to integrate SDL3 soon. The primary goal of this project is to create a fast, memory-efficient emulator that can run smoothly even on older hardware.

## Current Features
- Full implementation of all 501 GameBoy opcodes, with all instructions (excpet for EI) passing [SingleStepTest's SM83 Test Suite](https://github.com/SingleStepTests/sm83).
- Interrupt handling implemented and tested.
- Cycle counting for all instructions complete
- (Almost) all I/O registers memory-mapped.
- Basic implementation of the Timer (still needs to be tested).

## Planned Updates
- Basic implementation of the Pixel Processing Unit (PPU).

This project is in active development, so stay tuned for updates.