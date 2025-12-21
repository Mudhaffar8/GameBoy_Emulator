cmake_minimum_required(VERSION 4.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-O2") 

project(GAMEBOY_CPU_TESTS LANGUAGES CXX)

add_executable(main main.cpp cpu.cpp cpu.cpp memory.cpp memory.hpp tests.hpp json.hpp)