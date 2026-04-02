#pragma once
#include "../process/process.h"
#include <initializer_list>
#include <optional>
#include <iostream>
#include <windows.h>

namespace Core {

class MemoryManager {
public:
  // ── Read<T> ── Uses NtReadVirtualMemory (not ReadProcessMemory)
  // Returns zero-filled buffer if read fails (kept for API compatibility)
  template <typename T> static T Read(uintptr_t address) {
    auto result = ReadOptional<T>(address);
    if (result.has_value()) {
      return *result;
    }
    return T{}; // Return zero-initialized buffer for failed reads
  }

  // ── ReadOptional<T> ── Returns std::optional<T> so callers can distinguish
  // between "read zero" and "read failed"
  template <typename T> static std::optional<T> ReadOptional(uintptr_t address) {
    T buffer{};
    // Validate address before reading (prevent crashes and suspicious patterns)
    if (address < 0x10000 || address > 0x7FFFFFFFFFFFFFFF) {
      return std::nullopt;
    }
    NTSTATUS status = Process::NtRead(reinterpret_cast<void *>(address), &buffer, sizeof(T));
    if (status != 0) {
      return std::nullopt;
    }
    return buffer;
  }

  // ── Write<T> ── Still uses WriteProcessMemory (we only write when needed)
  template <typename T> static bool Write(uintptr_t address, const T &value) {
    // Validate address before writing
    if (address < 0x10000 || address > 0x7FFFFFFFFFFFFFFF) {
      return false;
    }
    return WriteProcessMemory(Process::GetHandle(),
                              reinterpret_cast<LPVOID>(address), &value,
                              sizeof(T), nullptr) != 0;
  }

  // ── ReadChain ── Helper for deep pointer chains
  template <typename T>
  static T ReadChain(uintptr_t base, std::initializer_list<uintptr_t> offsets) {
    uintptr_t current = base;
    if (offsets.size() == 0)
      return Read<T>(current);
    auto it = offsets.begin();
    while (it != offsets.end() - 1) {
      current = Read<uintptr_t>(current + *it);
      if (current < 0x10000 || current > 0x7FFFFFFFFFFFFFFF)
        return T{};
      ++it;
    }
    return Read<T>(current + *it);
  }

  // ── ReadBatch ── Read a contiguous block of memory
  static bool ReadBatch(uintptr_t address, void* buffer, size_t size) {
    if (address < 0x10000 || address > 0x7FFFFFFFFFFFFFFF) return false;
    return Process::NtRead(reinterpret_cast<void*>(address), buffer, size) == 0;
  }

  // ── ReadRaw ── Batch NtReadVirtualMemory call
  static bool ReadRaw(uintptr_t address, void *buffer, size_t size) {
    // Validate address before reading
    if (address < 0x10000 || address > 0x7FFFFFFFFFFFFFFF || buffer == nullptr || size == 0) {
      return false;
    }
    NTSTATUS status = Process::NtRead(reinterpret_cast<void *>(address), buffer, size);
    if (status != 0) {
      std::cerr << "[MemoryManager::ReadRaw] FAILED at address 0x" << std::hex
                << address << " size=" << std::dec << size
                << " NTSTATUS=0x" << std::hex << status << std::dec << std::endl;
    }
    return status == 0;
  }
};

} // namespace Core
