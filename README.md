# GameBoy Emulator (WIP)

This is a work-in-progress GameBoy emulator written in C++. The primary goal of this project is to create a fast, memory-efficient emulator that can run smoothly even on older hardware.

## Current Features
- Full implementation of all 501 GameBoy opcodes, with all instructions (excpet for EI) passing [SingleStepTest's SM83 Test Suite](https://github.com/SingleStepTests/sm83). For more info, check out sm83-tests branch.
- Interrupt handling implemented and tested.
- Cycle counting for all instructions complete
- (Almost) all I/O registers memory-mapped.
- Basic implementation of the Timer (still needs to be tested).
- Some PPU features supported:
  - Per-scanline rendering
  - Full-frame rendering (Rendering multiple scanlines in succession)
  - Display can be outputted through SDL3
 
## Planned Updates
- Add more PPU features:
  - X and Y scrolling behaviours
  - Implement all the above PPU features for the Window layer
  - Sprite Rendering


This project is in active development, so stay tuned for updates.
