#include "tests.hpp"
#include "cpu.hpp"
#include "memory.hpp"

int main(int argc, char** argv)
{    
    Mmu mem;
    Cpu cpu(&mem);

    // Non-prefixed Instructions
    GBTests::run_tests(cpu, mem, 0x00, 0x80, {0x69}); // Load and various other operations
    GBTests::run_tests(cpu, mem, 0x80, 0xC0); // Arithmetic/Logic operations
    GBTests::run_tests(cpu, mem, 0xC0, 0xE0, {0xD9}); // Mostly stack operations & Branching operations
    GBTests::run_tests(cpu, mem, 0xE0, 0x100, {0xFB}); // More stack operations

    // CB-prefixed Instructions
    GBTests::run_tests(cpu, mem, 0x00, 0x100, {}, true);

    std::cout << "\nSuccessfully passed all tests!";

    return 0;
}