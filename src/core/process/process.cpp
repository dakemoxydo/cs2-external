#include "process.h"
#include <tlhelp32.h>

namespace Core {
HANDLE Process::hProcess = nullptr;
DWORD Process::processId = 0;

bool Process::Attach(const std::wstring &processName) {
  HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hSnap == INVALID_HANDLE_VALUE)
    return false;

  PROCESSENTRY32W procEntry;
  procEntry.dwSize = sizeof(procEntry);

  if (Process32FirstW(hSnap, &procEntry)) {
    do {
      if (!_wcsicmp(procEntry.szExeFile, processName.c_str())) {
        processId = procEntry.th32ProcessID;
        CloseHandle(hSnap);

        hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE |
                                   PROCESS_VM_OPERATION,
                               FALSE, processId);
        if (hProcess == NULL) {
          processId = 0;
          return false;
        }
        return true;
      }
    } while (Process32NextW(hSnap, &procEntry));
  }

  CloseHandle(hSnap);
  return false;
}

void Process::Detach() {
  if (hProcess) {
    CloseHandle(hProcess);
    hProcess = nullptr;
  }
  processId = 0;
}

HANDLE Process::GetHandle() { return hProcess; }

DWORD Process::GetProcessId() { return processId; }
} // namespace Core
