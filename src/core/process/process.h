#pragma once
#include <string>
#include <windows.h>


namespace Core {

// Signature of NtReadVirtualMemory (from ntdll)
using NtReadVirtualMemoryFn = NTSTATUS(NTAPI *)(HANDLE ProcessHandle,
                                                PVOID BaseAddress, PVOID Buffer,
                                                SIZE_T NumberOfBytesToRead,
                                                PSIZE_T NumberOfBytesRead);

class Process {
public:
  static bool Attach(const std::wstring &processName);
  static void Detach();
  static HANDLE GetHandle();
  static DWORD GetProcessId();

  /// Direct NtReadVirtualMemory call (replaces ReadProcessMemory).
  static bool NtRead(void *address, void *buffer, size_t size);

private:
  static HANDLE hProcess;
  static DWORD processId;
  static NtReadVirtualMemoryFn s_ntRvm;

  /// Try to steal a handle to targetPid from trusted processes (Steam etc.)
  static HANDLE TryStealHandle(DWORD targetPid);

  /// Resolve ntdll exports once.
  static void ResolveNtFunctions();
};

} // namespace Core
