#pragma once
#include "../process/process.h"
#include <vector>
#include <windows.h>

namespace Core {
class MemoryManager {
public:
  template <typename T> static T Read(uintptr_t address) {
    T buffer{};
    ReadProcessMemory(Process::GetHandle(), reinterpret_cast<LPCVOID>(address),
                      &buffer, sizeof(T), nullptr);
    return buffer;
  }

  template <typename T> static bool Write(uintptr_t address, const T &value) {
    return WriteProcessMemory(Process::GetHandle(),
                              reinterpret_cast<LPVOID>(address), &value,
                              sizeof(T), nullptr) != 0;
  }

  static bool ReadRaw(uintptr_t address, void *buffer, size_t size) {
    return ReadProcessMemory(Process::GetHandle(),
                             reinterpret_cast<LPCVOID>(address), buffer, size,
                             nullptr) != 0;
  }
};
} // namespace Core
