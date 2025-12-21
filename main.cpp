#include "tests.hpp"

int main(int argc, char** argv)
{    
    // Non-prefixed Instructions
    GBTests::run_tests(0x00, 0x80, {0x69}); // Load and various other operations
    GBTests::run_tests(0x80, 0xC0); // Arithmetic/Logic operations
    GBTests::run_tests(0xC0, 0xE0); // Mostly stack operations & Branching operations
    GBTests::run_tests(0xE0, 0x100, {0xFB}); // More stack operations

    // CB-prefixed Instructions
    GBTests::run_tests(0x00, 0x100, {}, true);

    return 0;
}