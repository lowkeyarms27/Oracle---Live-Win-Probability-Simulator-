cmake_minimum_required(VERSION 3.20)
project(chip8-emu VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SDL2
find_package(SDL2 REQUIRED)

# Catch2 via FetchContent
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.6.0
)
FetchContent_MakeAvailable(Catch2)

# Core library (shared between emulator and tests)
add_library(chip8_core STATIC
    src/chip8.cpp
    src/display.cpp
    src/input.cpp
    src/audio.cpp
    src/rom.cpp
)
target_include_directories(chip8_core PUBLIC src)
target_link_libraries(chip8_core PUBLIC SDL2::SDL2)

# Emulator executable
add_executable(chip8-emu src/main.cpp)
target_link_libraries(chip8-emu PRIVATE chip8_core SDL2::SDL2main)

# Tests
add_subdirectory(tests)
