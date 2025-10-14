#include <iostream>

#include "cpu.hpp"
#include "memory.hpp"

int main(int argc, char** argv)
{
    Memory mem;
    Cpu cpu(&mem);

    if (!mem.load_rom("./roms/test.txt"))
    {
        std::cerr << "FAIL";
        return 1;
    }

    for (int i = 0; i < 400; ++i)
        std::cout << mem.read_byte(i);

    return 0;
}