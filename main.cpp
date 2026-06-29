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

    std::string save_file{};
    if (argc > 2)
        save_file = std::string(argv[2]);
    
    Gameboy gameboy("./test_roms/" + rom_file, save_file);
    gameboy.run();

    return 0;
}