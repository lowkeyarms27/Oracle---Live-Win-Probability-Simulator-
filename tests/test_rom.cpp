#include <catch2/catch_test_macros.hpp>
#include "rom.hpp"
#include <fstream>
#include <cstdio>

TEST_CASE("ROM::validate rejects non-existent file", "[rom]") {
    REQUIRE_FALSE(ROM::validate("no_such_file.ch8"));
}

TEST_CASE("ROM::load returns empty for bad path", "[rom]") {
    auto data = ROM::load("no_such_file.ch8");
    REQUIRE(data.empty());
}

TEST_CASE("ROM::validate accepts a valid small ROM", "[rom]") {
    const char* tmp = "test_temp.ch8";
    {
        std::ofstream f(tmp, std::ios::binary);
        uint8_t bytes[] = {0x00, 0xE0}; // CLS opcode
        f.write(reinterpret_cast<char*>(bytes), sizeof(bytes));
    }
    REQUIRE(ROM::validate(tmp));
    std::remove(tmp);
}
