#pragma once
#include "../process/process.h"
#include "core/constants.h"
#include <initializer_list>
#include <optional>
#include <windows.h>

namespace Core {

class MemoryManager {
public:
  template <typename T> static T Read(uintptr_t address) {
    auto result = ReadOptional<T>(address);
    if (result.has_value()) {
      return *result;
    }
    return T{};
  }

  template <typename T> static std::optional<T> ReadOptional(uintptr_t address) {
    T buffer{};
    if (address < Constants::MIN_VALID_ADDRESS || address > Constants::MAX_VALID_ADDRESS) {
      return std::nullopt;
    }
    NTSTATUS status = Process::NtRead(reinterpret_cast<void *>(address), &buffer, sizeof(T));
    if (status != 0) {
      return std::nullopt;
    }
    return buffer;
  }

  template <typename T> static bool Write(uintptr_t address, const T &value) {
    if (address < Constants::MIN_VALID_ADDRESS || address > Constants::MAX_VALID_ADDRESS) {
      return false;
    }
    return Process::NtWrite(reinterpret_cast<void *>(address), &value, sizeof(T));
  }

  template <typename T>
  static T ReadChain(uintptr_t base, std::initializer_list<uintptr_t> offsets) {
    uintptr_t current = base;
    if (offsets.size() == 0)
      return Read<T>(current);
    auto it = offsets.begin();
    while (it != offsets.end() - 1) {
      current = Read<uintptr_t>(current + *it);
      if (current < Constants::MIN_VALID_ADDRESS || current > Constants::MAX_VALID_ADDRESS)
        return T{};
      ++it;
    }
    return Read<T>(current + *it);
  }

  static bool ReadRaw(uintptr_t address, void *buffer, size_t size) {
    if (address < Constants::MIN_VALID_ADDRESS || address > Constants::MAX_VALID_ADDRESS || buffer == nullptr || size == 0) {
      return false;
    }
    NTSTATUS status = Process::NtRead(reinterpret_cast<void *>(address), buffer, size);
    return status == 0;
  }
};

} // namespace Core
