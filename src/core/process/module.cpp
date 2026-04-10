#include "module.h"
#include "process.h"
#include <tlhelp32.h>


namespace Core {
uintptr_t Module::GetBaseAddress(const std::wstring &moduleName) {
  HANDLE hSnap = CreateToolhelp32Snapshot(
      TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, Process::GetProcessId());
  if (hSnap == INVALID_HANDLE_VALUE) {
    return 0;
  }

  MODULEENTRY32W modEntry{};
  modEntry.dwSize = sizeof(modEntry);
  if (Module32FirstW(hSnap, &modEntry)) {
    do {
      if (!_wcsicmp(modEntry.szModule, moduleName.c_str())) {
        CloseHandle(hSnap);
        return reinterpret_cast<uintptr_t>(modEntry.modBaseAddr);
      }
    } while (Module32NextW(hSnap, &modEntry));
  }

  CloseHandle(hSnap);
  return 0;
}
} // namespace Core
