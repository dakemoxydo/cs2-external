#include "pattern_scanner.h"
#include "../process/process.h"
#include <cctype>
#include <cstring>
#include <optional>
#include <sstream>
#include <tlhelp32.h>
#include <vector>

namespace Core {

namespace {

struct ModuleInfo {
  uintptr_t base = 0;
  size_t size = 0;
};

std::optional<ModuleInfo> GetRemoteModuleInfo(const std::wstring &moduleName) {
  HANDLE snapshot =
      CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
                               Process::GetProcessId());
  if (snapshot == INVALID_HANDLE_VALUE) {
    return std::nullopt;
  }

  MODULEENTRY32W entry{};
  entry.dwSize = sizeof(entry);
  if (Module32FirstW(snapshot, &entry)) {
    do {
      if (!_wcsicmp(entry.szModule, moduleName.c_str())) {
        CloseHandle(snapshot);
        return ModuleInfo{reinterpret_cast<uintptr_t>(entry.modBaseAddr),
                          static_cast<size_t>(entry.modBaseSize)};
      }
    } while (Module32NextW(snapshot, &entry));
  }

  CloseHandle(snapshot);
  return std::nullopt;
}

std::vector<int> ParseSignature(const std::string &signature) {
  std::vector<int> bytes;
  std::istringstream stream(signature);
  std::string token;
  while (stream >> token) {
    if (token == "?" || token == "??") {
      bytes.push_back(-1);
      continue;
    }

    for (char &ch : token) {
      ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }

    bytes.push_back(std::stoi(token, nullptr, 16));
  }
  return bytes;
}

} // namespace

uintptr_t PatternScanner::FindPattern(const std::wstring &moduleName,
                                      const std::string &signature) {
  const auto moduleInfo = GetRemoteModuleInfo(moduleName);
  if (!moduleInfo.has_value() || moduleInfo->base == 0 || moduleInfo->size == 0) {
    return 0;
  }

  const std::vector<int> pattern = ParseSignature(signature);
  if (pattern.empty() || pattern.size() > moduleInfo->size) {
    return 0;
  }

  std::vector<uint8_t> buffer(moduleInfo->size);
  // Read the module in chunks instead of one giant NtReadVirtualMemory.
  // Large single reads frequently return STATUS_PARTIAL_COPY (or are reported
  // as partial) when they span invalid/guard pages, which would make the whole
  // scan fail even though the pattern is present. Reading in pages lets the
  // rest of the module stay scannable; a chunk that can't be read is left
  // zeroed so only patterns crossing it are missed.
  constexpr size_t kChunkSize = 256 * 1024;
  uint8_t *cursor = buffer.data();
  size_t remaining = moduleInfo->size;
  uintptr_t curBase = moduleInfo->base;
  while (remaining > 0) {
    const size_t chunk = (std::min)(remaining, kChunkSize);
    if (Process::NtRead(reinterpret_cast<void *>(curBase), cursor, chunk) != 0) {
      std::memset(cursor, 0, chunk);
    }
    cursor += chunk;
    curBase += chunk;
    remaining -= chunk;
  }

  const size_t last = buffer.size() - pattern.size();
  for (size_t i = 0; i <= last; ++i) {
    bool matched = true;
    for (size_t j = 0; j < pattern.size(); ++j) {
      if (pattern[j] != -1 && buffer[i + j] != static_cast<uint8_t>(pattern[j])) {
        matched = false;
        break;
      }
    }

    if (matched) {
      return moduleInfo->base + i;
    }
  }

  return 0;
}

} // namespace Core
