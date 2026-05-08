#pragma once
#include <string>
#include <vector>

class ROM {
public:
    static bool validate(const std::string& path);
    static std::vector<uint8_t> load(const std::string& path);
};
