#pragma once
#include "../process/process.h"
#include <windows.h>

namespace Core {

class MemoryManager {
public:
  // ── Read<T> ── Uses NtReadVirtualMemory (not ReadProcessMemory)
  template <typename T> static T Read(uintptr_t address) {
    T buffer{};
    Process::NtRead(reinterpret_cast<void *>(address), &buffer, sizeof(T));
    return buffer;
  }

  // ── Write<T> ── Still uses WriteProcessMemory (we only write when needed)
  template <typename T> static bool Write(uintptr_t address, const T &value) {
    return WriteProcessMemory(Process::GetHandle(),
                              reinterpret_cast<LPVOID>(address), &value,
                              sizeof(T), nullptr) != 0;
  }

  // ── ReadRaw ── Batch NtReadVirtualMemory call
  static bool ReadRaw(uintptr_t address, void *buffer, size_t size) {
    return Process::NtRead(reinterpret_cast<void *>(address), buffer, size);
  }
};

} // namespace Core
