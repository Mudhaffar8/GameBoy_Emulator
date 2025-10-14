#include <iostream>
#include <fstream>
#include <cassert>
#include <stdexcept>
#include <sstream>

#include "cpu.hpp"
#include "json.hpp"

using json = nlohmann::json;

int main(int argc, char** argv)
{
    Memory mem;
    Cpu cpu(&mem);


    return 0;
}