#include <SDL3/SDL.h>
#include <SDL3/SDL_keyboard.h>

#include <assert.h>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>

#include "cpu.hpp"
#include "display.hpp"
#include "memory.hpp"
#include "ppu.hpp"
#include "timer.hpp"

/* Speed Tests */
double cpu_speed_test();
void cpu_avg_speed_test();
void ppu_speed_test();

/* Full Frame Tests */
void ppu_window_frame_test(int scx, int scy);
void ppu_sprite_frame_test(int sprite_x, int sprite_y);

/* Scrolling tests w/ Window & BG */
void ppu_sprite_scroll_test(uint8_t tile_num, uint8_t attrib);
void ppu_scroll_test(int scx, int scy); // Renders one frame with scrolling applied
void ppu_scroll_test2(); 

/* Scanline Tests */
void ppu_window_scanline_test(int scx, int scy);
void ppu_bg_scanline_test(int scx, int scy);
void ppu_scanline_test(int scx, int scy);

/* Palette Tests */
void bg_window_palette_tests(uint8_t palette);
void sprite_palette_tests(uint8_t tile_num, uint8_t palette);

/* Priority Tests */
void priority_test();

/* Interrupt Test */
void interrupt_test();

/* Enable Test */
void enable_disable_bg_test();

/* For Testing ROMs */
void rom_test();

static Mmu mmu;
static Cpu cpu(mmu);
static Ppu ppu(mmu);
static Display display(ppu);
static Timer timer(mmu);

int main(int argc, char** argv)
{
    int arg1 = std::atoi(argv[2]);
    int arg2 = std::atoi(argv[3]);
    bool debug_mode_enabled = std::string(argv[4]) == "t";

    ppu.debug_mode = debug_mode_enabled;

    mmu.write_byte(0b11100100, BG_PALETTE);
    mmu.write_byte(0b11100100, OBJ_PALETTE_0);
    mmu.write_byte(0b11100100, OBJ_PALETTE_1);
    
    ppu.set_lcd_status(Ppu::LCDStatus::Unused, true);

    ppu.set_lcdc(Ppu::LCDC::LCDPpuEnable, true);
    ppu.set_lcdc(Ppu::LCDC::BgWindowEnable, true);
    ppu.set_lcdc(Ppu::LCDC::BgWindowTileDataArea, true);
    ppu.set_lcdc(Ppu::LCDC::WindowTileMap, true);

    //std::cout << "Initialized before program starts ^\n";
    
    if (argv[1] == std::string("scroll_test"))
        ppu_scroll_test(arg1, arg2);
    else if (argv[1] == std::string("scroll_test2"))
        ppu_scroll_test2();
    else if (argv[1] == std::string("window_scanline_test"))
        ppu_window_scanline_test(arg1, arg2);
    else if (argv[1] == std::string("scanline_test"))
        ppu_scanline_test(arg1, arg2);
    else if (argv[1] == std::string("window_test"))
        ppu_window_frame_test(arg1, arg2);
    else if (argv[1] == std::string("sprite_test"))
        ppu_sprite_frame_test(arg1, arg2);
    else if (argv[1] == std::string("sprite_scroll"))
        ppu_sprite_scroll_test(arg1, arg2);
    else if (argv[1] == std::string("palette_test"))
        bg_window_palette_tests(arg1);
    else if (argv[1] == std::string("sprite_palette"))
        sprite_palette_tests(arg1, arg2);
    else if (argv[1] == std::string("priority_test"))
        priority_test();
    else if (argv[1] == std::string("ppu_speed_test"))
        ppu_speed_test();
    else if (argv[1] == std::string("rom_test")) 
        rom_test();
    else if (argv[1] == std::string("interrupt_test"))
        interrupt_test();
    else if (argv[1] == std::string("enable_bg_test"))
        enable_disable_bg_test();
        
    return 0;
}

void enable_disable_bg_test()
{
    ppu.set_lcdc(Ppu::LCDC::BgWindowEnable, true);
    ppu.set_lcdc(Ppu::LCDC::BgTileMapArea, true);
    ppu.set_lcdc(Ppu::LCDC::BgWindowTileDataArea, true);
    
    while (display.is_program_running())
    {
        display.handle_input();
        ppu.render_frame();
        display.update_screen();
        
        SDL_Delay(1000);
        
        ppu.set_lcdc(
            Ppu::LCDC::BgWindowEnable, 
            !ppu.check_lcdc(Ppu::LCDC::BgWindowEnable)
        );
    }
}

void interrupt_test()
{
    cpu.get_ime() = true;

    uint32_t cycles_elapsed = 0;
    while (cycles_elapsed <= GBTiming::CYCLES_PER_FRAME)
    {
        uint32_t cycles = cpu.execute_instruction();
        ppu.tick(cycles);

        cycles_elapsed += cycles;
    }

    display.update_screen();

    std::cout << "Interrupt Flag: " << std::hex << +mmu.read_byte(INTERRUPT_FLAG) << '\n';
    std::cout << "Interrupt Enable: " << std::hex << +mmu.read_byte(INTERRUPT_ENABLE);

    SDL_Delay(3000);
}

void rom_test()
{
    mmu.load_rom("./test_roms/02-interrupts.gb");
    bool is_running = true;

    uint32_t cycles_elapsed = 0;
    while (display.is_program_running())
    {
        display.handle_input();

        auto start = std::chrono::steady_clock::now();
        
        while (cycles_elapsed <= GBTiming::CYCLES_PER_FRAME)
        {
            uint32_t cycles = cpu.execute_instruction();
            ppu.tick(cycles);
            timer.tick(cycles);

            cycles_elapsed += cycles;
        }

        cycles_elapsed %= GBTiming::CYCLES_PER_FRAME;

        display.update_screen();

        auto end = std::chrono::steady_clock::now();
        double diff = std::chrono::duration<double, std::milli>(end - start).count();

        uint32_t time = (diff < 16.67f) ? diff : 16.67f;
        SDL_Delay(time);
    }
}

// Both Window, BG drawing On top of each other
// OAM buffer with 40 entries in 8x16 mode
// Most computationally expensive scenario
void ppu_speed_test()
{
    ppu.set_lcdc(Ppu::LCDC::BgWindowEnable, true);
    ppu.set_lcdc(Ppu::LCDC::BgWindowTileDataArea, true);
    ppu.set_lcdc(Ppu::LCDC::WindowEnable, true);
    ppu.set_lcdc(Ppu::LCDC::WindowTileMap, true);
    ppu.set_lcdc(Ppu::LCDC::ObjEnable, true);
    ppu.set_lcdc(Ppu::LCDC::ObjSize, true);

    for (int i = 0; i < 40; ++i)
    {
        mmu.write_byte(16 + 5 * i, OAM_START + (4 * i));
        mmu.write_byte(8 + 5 * i, OAM_START + (4 * i) + 1);
        mmu.write_byte(i % 6, OAM_START + (4 * i) + 2);
        mmu.write_byte(0x00, OAM_START + (4 * i) + 3);
    }

    auto start = std::chrono::steady_clock::now();

    ppu.render_frame();

    auto ppu_end = std::chrono::steady_clock::now();
    double ppu_diff = std::chrono::duration<double, std::milli>(ppu_end - start).count();

    display.update_screen();
    
    auto end = std::chrono::steady_clock::now();
    double diff = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "PPU Speed: " << ppu_diff << " ms.\n";
    std::cout << "Total Speed: " << diff << " ms.\n";

    SDL_Delay(3000);
}

void priority_test()
{}

void bg_window_palette_tests(uint8_t palette)
{
    mmu.write_byte(palette, BG_PALETTE);
    
    ppu.render_bg_frame();
    ppu.render_window_frame();

    SDL_Delay(3000);
}

void sprite_palette_tests(uint8_t tile_num, uint8_t palette)
{
    mmu.write_byte(palette, OBJ_PALETTE_0);

    mmu.write_byte(40, OAM_START);
    mmu.write_byte(40, OAM_START + 1);
    mmu.write_byte(tile_num, OAM_START + 2);
    mmu.write_byte(0x00, OAM_START + 3);

    ppu.render_sprites_frame();

    SDL_Delay(3000);
}

void ppu_sprite_frame_test(int sprite_x, int sprite_y)
{
    mmu.write_byte(sprite_y, OAM_START);
    mmu.write_byte(sprite_x, OAM_START + 1);
    mmu.write_byte(0x00, OAM_START + 2);
    mmu.write_byte(0x00, OAM_START + 3);

    ppu.fill_white_screen();
    ppu.render_sprites_frame();
    display.update_screen();

    SDL_Delay(3000);
}

void ppu_sprite_scroll_test(uint8_t tile_num, uint8_t attrib)
{    
    for (int i = 0; i < GBResolution::WIDTH + 9; ++i)
    {
        mmu.write_byte(i, OAM_START);
        mmu.write_byte(i, OAM_START + 1);
        mmu.write_byte(tile_num, OAM_START + 2);
        mmu.write_byte(attrib, OAM_START + 3);

        ppu.render_sprites_frame();
        display.update_screen();

        ppu.fill_white_screen();

        SDL_Delay(16);
    }

    SDL_Delay(3000);
}

void ppu_scanline_test(int scx, int scy)
{
    ppu.set_window_scroll_values(scx, scy);

    for (int i = 0; i < GBResolution::HEIGHT; ++i)
    {
        std::cout << "Rendered BG Scanline #" << i;
        ppu.render_bg_scanline(i);
        SDL_Delay(500);

        display.update_screen();

        std::cout << "Rendered Window Scanline #" << i;
        ppu.render_window_scanline(i);
        SDL_Delay(500);

        display.update_screen();
    }

    SDL_Delay(2000);
}

void ppu_window_scanline_test(int scx, int scy)
{
    ppu.set_window_scroll_values(scx, scy);

    ppu.render_window_scanline(0);
    display.update_screen();

    SDL_Delay(3000);
}


void ppu_window_frame_test(int scx, int scy)
{
    ppu.set_window_scroll_values(scx, scy);

    ppu.render_frame();
    display.update_screen();

    SDL_Delay(3000);
}

void ppu_bg_scanline_test(int scx, int scy)
{
    ppu.set_bg_scroll_values(scx, scy);

    ppu.render_bg_scanline(0);
    display.update_screen();

    SDL_Delay(3000);
}

void ppu_bg_scroll_test(int scx, int scy)
{
    ppu.set_bg_scroll_values(scx, scy);

    ppu.set_lcdc(Ppu::LCDC::BgWindowEnable, true);

    ppu.render_frame();
    display.update_screen();

    SDL_Delay(3000);
}

void ppu_scroll_test(int scx, int scy)
{
    ppu.set_bg_scroll_values(scx, scy);
    ppu.set_window_scroll_values(scx, scy);

    ppu.render_frame();
    display.update_screen();

    SDL_Delay(3000);
}

void ppu_scroll_test2()
{  
    ppu.set_lcdc(Ppu::LCDC::WindowEnable, true);
    ppu.set_lcdc(Ppu::LCDC::ObjEnable, true);
    ppu.set_lcdc(Ppu::LCDC::ObjSize, true);

    ppu.render_frame();
    display.update_screen();

    SDL_Delay(1000);

    auto start = std::chrono::steady_clock::now();
    
    for (uint32_t i = 0; i < (GBResolution::TILE_MAP_SIZE_PIXELS * 2); ++i)
    {
        ppu.set_bg_scroll_values(i, i);
        ppu.set_window_scroll_values(7 + i, i);

        ppu.render_frame();
        display.update_screen();

        // ppu.reset_screen();

        // std::cout << "Scroll = (X: " << i << ", Y: " << i << ")\n";

        SDL_Delay(16);
    }

    auto end = std::chrono::steady_clock::now();
    double diff = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << diff;
    
    SDL_Delay(3000);
}

void cpu_avg_speed_test()
{
    constexpr int NUM_OF_TESTS = 1000;

    std::vector<double> times;
    times.reserve(NUM_OF_TESTS);
    
    for (int i = 0; i < NUM_OF_TESTS; ++i) 
        times.push_back(cpu_speed_test());   

    double avg = std::accumulate(times.begin(), times.end(), 0.0);
    avg /= static_cast<double>(NUM_OF_TESTS);

    std::cout << avg;
}

double cpu_speed_test()
{
    uint32_t buffer[GBResolution::HEIGHT * GBResolution::WIDTH]{};
    std::fill(buffer, buffer + sizeof(buffer) / sizeof(buffer[0]), GBColours::COLOUR_10);

    mmu.write_byte(0x27, 0); // DAA
    mmu.write_byte(0x0F, 1); // RLCA
    mmu.write_byte(0x99, 3); // SBC A, C
    mmu.write_byte(0xC7, 5); // RST 0x00

    auto start = std::chrono::steady_clock::now();
    for (uint32_t i = 0;  i < GBTiming::CYCLES_PER_FRAME; i += cpu.execute_instruction());

    display.update_screen(buffer); 

    auto end = std::chrono::steady_clock::now();
    double diff = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << diff;

    SDL_Delay(1000);
    
    return diff;
}