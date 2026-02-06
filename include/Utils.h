#pragma once

#include <string>
#include <vector>
#include <cstdint>

class Utils {
public:
    static bool createDirectory(const std::string& path);
    static std::string getLastErrorString();
    static void sleep(uint32_t milliseconds);
};