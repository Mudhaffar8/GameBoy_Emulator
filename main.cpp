#include <iostream>

#include "gameboy.hpp"

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << "Invalid # of Arguments!\n";
        exit(0);
    }

    std::string rom_file = std::string(argv[1]);

    Gameboy gameboy("./test_roms/" + rom_file);
    gameboy.run();

    return 0;
}