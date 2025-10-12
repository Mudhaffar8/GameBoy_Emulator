#include <iostream>
#include <fstream>
#include <cassert>
#include <stdexcept>
#include <sstream>

#include "cpu.hpp"
#include "json.hpp"

using json = nlohmann::json;

void check_val(uint8_t val, uint8_t other_val, const char* name)
{   
    if (val != other_val) 
    {
        std::cout << name << " Expected: " << +other_val << " Got: " << +val << '\n';

        throw std::runtime_error(std::string("Fail on ") + name);
    }
}

void check_val(uint16_t val, uint16_t other_val, const char* name)
{   
    if (val != other_val) 
    {
        std::cout << name << " Expected: " << +other_val << " Got: " << +val << '\n';

        throw std::runtime_error(std::string("Fail on ") + name);
    }
}

std::string int_to_hex(int i)
{
    std::stringstream stream;
    stream << std::hex << i;
    return stream.str();
}

int main(int argc, char** argv)
{
    Memory mem;
    Cpu cpu(&mem);

    // cpu.test();

#if 1
    int skip_tests[] = { 0x1000 /* 0xFB, 0x69 */};
    for (int i = 0x00; i < 0x100; ++i)
    {
        bool skip = false;
        for (int test : skip_tests)
        { 
            if (test == i) 
            {
                skip = true;
                break;
            }
        }

        if (skip) continue;

        std::string filename = "v1/cb " + (i < 0x10 ? std::string("0") : "") + int_to_hex(i) + ".json";
        std::ifstream file(filename);

        if (!file) continue;

        json data = json::parse(file);

        int number = 1;
        for (json::iterator it = data.begin(); it != data.end(); ++it)
        {

            cpu.A = static_cast<uint8_t>((*it)["initial"]["a"]);
            cpu.F  = static_cast<uint8_t>((*it)["initial"]["f"]);
            cpu.BC.high = static_cast<uint8_t>((*it)["initial"]["b"]);
            cpu.BC.low = static_cast<uint8_t>((*it)["initial"]["c"]);
            cpu.DE.high = static_cast<uint8_t>((*it)["initial"]["d"]);
            cpu.DE.low = static_cast<uint8_t>((*it)["initial"]["e"]);
            cpu.HL.high = static_cast<uint8_t>((*it)["initial"]["h"]);
            cpu.HL.low = static_cast<uint8_t>((*it)["initial"]["l"]);
            cpu.PC = static_cast<uint16_t>((*it)["initial"]["pc"]);
            cpu.SP = static_cast<uint16_t>((*it)["initial"]["sp"]);

            cpu.is_halted = false;

            // uint8_t IME = (*it)["initial"]["ime"];
            // cpu.IME = static_cast<bool>(IME);

            // uint8_t IE = static_cast<uint8_t>((*it)["initial"]["ie"]);
            // mem.write_byte(IE, INTERRUPT_ENABLE);
        
            // Set initial RAM values
            for (const auto& ram_write : ((*it)["initial"]["ram"]))
            {
                uint8_t byte = static_cast<uint8_t>(ram_write[1]);
                uint16_t addr = static_cast<uint16_t>(ram_write[0]);

                mem.write_byte(byte, addr);
            }

            uint32_t cycles = cpu.execute_instruction();


            check_val(cpu.A, static_cast<uint8_t>((*it)["final"]["a"]), "A");
            check_val(cpu.F, static_cast<uint8_t>((*it)["final"]["f"]), "F");
            check_val(cpu.BC.high, static_cast<uint8_t>((*it)["final"]["b"]), "B");
            check_val(cpu.BC.low, static_cast<uint8_t>((*it)["final"]["c"]), "C");
            check_val(cpu.DE.high, static_cast<uint8_t>((*it)["final"]["d"]), "D");
            check_val(cpu.DE.low, static_cast<uint8_t>((*it)["final"]["e"]), "E");
            check_val(cpu.HL.high, static_cast<uint8_t>((*it)["final"]["h"]), "H");
            check_val(cpu.HL.low, static_cast<uint8_t>((*it)["final"]["l"]), "L");
            check_val(cpu.PC, static_cast<uint16_t>((*it)["final"]["pc"]), "PC");
            check_val(cpu.SP, static_cast<uint16_t>((*it)["final"]["sp"]), "SP");

            // IME = static_cast<uint8_t>((*it)["final"]["ime"]);
            // check_val(static_cast<uint8_t>(cpu.IME), IME, "IME");

            // IE = static_cast<uint8_t>((*it)["final"]["ie"]);
            // check_val(mem.read_byte(INTERRUPT_ENABLE), IE, "IE");

            // Compare RAM values
            for (const auto& ram_write : ((*it)["final"]["ram"]))
            {
                uint8_t byte = static_cast<uint8_t>(ram_write[1]);
                uint16_t addr = static_cast<uint16_t>(ram_write[0]);
                
                assert(mem.read_byte(addr) == byte);
            }

            // std::cout << "Passed Test  #" << number++ << '\n';
        }

        std::cout << "Passed Test: " << filename << '\n';
    }
#endif

    return 0;
}