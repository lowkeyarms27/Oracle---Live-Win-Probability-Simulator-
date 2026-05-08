add_executable(chip8_tests
    test_cpu.cpp
    test_display.cpp
    test_input.cpp
    test_rom.cpp
)
target_link_libraries(chip8_tests PRIVATE chip8_core Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(chip8_tests)
