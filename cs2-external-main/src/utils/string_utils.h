#pragma once
#include <string>
#include <algorithm>

namespace Utils {

inline std::string ToLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return ::tolower(c); });
    return str;
}

inline std::string ToUpper(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return ::toupper(c); });
    return str;
}

inline std::string ToUpper(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return ::toupper(c); });
    return str;
}

inline bool StartsWith(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

inline bool EndsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

inline std::string Trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

} // namespace Utils
