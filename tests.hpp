#include <iostream>
#include <fstream>
#include <cassert>
#include <stdexcept>
#include <sstream>
#include <string>
#include <type_traits>

#include "cpu.hpp"
#include "json.hpp"

using json = nlohmann::json;


namespace GBTests
{
    /// @brief Compares two values and throws an exception if they differ.
    /// @tparam T type of values being compared. Must support equality comparison.
    /// @param val Value being checked.
    /// @param other_val Value compared against.
    /// @param name Descriptive label used in error output and exception messages.
    /// @throws `std::runtime_error` thrown when values are not equal
    template<typename T>
    void check_val(T val, T other_val, std::string name)
    {   
        if (val != other_val) 
        {
            // To prevent printing the ascii character instead of number for uint8_t, uint16_t etc.
            if constexpr (std::is_arithmetic<T>::value)
                std::cout << name << " Expected: " << +other_val << " Got: " << +val << '\n';
            else
                std::cout << name << " Expected: " << other_val << " Got: " << val << '\n';
            
            throw std::runtime_error(std::string("Fail on ") + name);
        }
    }

    /// @brief Converts integer into hexidecimal string representation.
    /// @param num The integer value to convert.
    /// @return A lowercase hexadecimal string representing the input number.
    std::string int_to_hex(int num)
    {
        std::stringstream stream;

        if (num < 0x10) stream << "0";
        stream << std::hex << num;

        return stream.str();
    }

    /// @brief Runs instruction tests over a specified opcode range.
    /// @param start First opcode (inclusive). Must be > 0x00.
    /// @param end Last opcode (exclusive). Must be < 0x100.
    /// @param instructions_to_skip List of opcodes not tested.
    /// @param is_cb If true, tester runs CB-prefixed opcodes instead.
    /// @note Assertions enforce that start < end, start > 0x00, and end < 0x100.
    void run_tests(
        int start = 0x00, 
        int end = 0x100, 
        const std::vector<int>& instructions_to_skip = {},
        bool is_cb = false
    )
    {
        assert(start < end);
        assert(end <= 0x100);
        assert(start >= 0x0);

        Memory mem;
        Cpu cpu(&mem);

        for (int i = start; i < end; ++i)
        {
            // Skip if current instructions should be skipped
            bool skip = false;
            for (int test : instructions_to_skip)
            { 
                if (test == i) 
                {
                    skip = true;
                    break;
                }
            }
            if (skip) continue;

            std::string file_name = "v1/" + 
                (is_cb ? std::string("cb ") : std::string()) + 
                int_to_hex(i) + ".json";
            std::ifstream file(file_name);

            if (!file) continue;

            json data = json::parse(file);

            int number = 1;
            for (json::iterator it = data.begin(); it != data.end(); ++it)
            {
                // Set CPU register values
                // TODO: Fix encapsulation (make namespace into class and make it into friend class?)
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

                // Compare CPU register values
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

                // May cause Interrupt Handler to be called
                // IME = static_cast<uint8_t>((*it)["final"]["ime"]);
                // check_val(static_cast<uint8_t>(cpu.IME), IME, "IME");

                // May not work - the interrupt enable register is sometimes written as "ie" or "ei" in the JSON test file
                // IE = static_cast<uint8_t>((*it)["final"]["ie"]);
                // check_val(mem.read_byte(INTERRUPT_ENABLE), IE, "IE");

                // Compare RAM values
                for (const auto& ram_write : ((*it)["final"]["ram"]))
                {
                    uint8_t byte = static_cast<uint8_t>(ram_write[1]);
                    uint16_t addr = static_cast<uint16_t>(ram_write[0]);
                    
                    check_val(mem.read_byte(addr), byte, std::string("ram @ ") + std::string(1, addr));
                }

                // std::cout << "Passed Test  #" << number++ << '\n';
            }

            std::cout << "Passed Test: " << file_name << '\n';
        }
    }
}