#include <iostream>
#include "cpu.hpp"

int main(int argc, char** argv)
{
    Memory mem;
    Cpu cpu(&mem);

    cpu.test();

    return 0;
}