#include "pattern_scanner.h"
#include "../process/process.h"
#include <cctype>
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
  if (Process::NtRead(reinterpret_cast<void *>(moduleInfo->base), buffer.data(),
                      buffer.size()) != 0) {
    return 0;
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
