#pragma once
#include <string>
#include <windows.h>
#include <winternl.h>
#include <atomic>
#include <mutex>


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
  /// Returns NTSTATUS: 0 (STATUS_SUCCESS) on success, error code on failure.
  static NTSTATUS NtRead(void *address, void *buffer, size_t size);

private:
  static std::mutex s_mutex;
  static HANDLE hProcess;
  static std::atomic<DWORD> processId;
  static NtReadVirtualMemoryFn s_ntRvm;

  /// Try to steal a handle to targetPid from trusted processes (Steam etc.)
  static HANDLE TryStealHandle(DWORD targetPid);

  /// Resolve ntdll exports once.
  static void ResolveNtFunctions();
};

} // namespace Core
