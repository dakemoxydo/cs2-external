#pragma once

#include <string>

class License {
public:
    static bool validate();
    static void saveKey(const std::string& key);
    static std::string loadKey();
};
