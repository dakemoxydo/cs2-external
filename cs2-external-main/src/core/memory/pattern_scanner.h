#pragma once
#include <string>
#include <vector>
#include <windows.h>


namespace Core {
class PatternScanner {
public:
  static uintptr_t FindPattern(const std::wstring &moduleName,
                               const std::string &signature);
};
} // namespace Core
