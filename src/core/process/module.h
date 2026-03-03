#pragma once
#include <string>
#include <windows.h>


namespace Core {
class Module {
public:
  static uintptr_t GetBaseAddress(const std::wstring &moduleName);
};
} // namespace Core
