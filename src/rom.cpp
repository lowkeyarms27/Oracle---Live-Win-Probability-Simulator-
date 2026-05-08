#include "rom.hpp"
#include <fstream>

bool ROM::validate(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return false;
    auto size = f.tellg();
    return size > 0 && size <= (4096 - 0x200);
}

std::vector<uint8_t> ROM::load(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), {}};
}
