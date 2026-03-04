#include "stealth.h"
#include <cstring>
#include <cwchar>
#include <winternl.h> // PROCESS_BASIC_INFORMATION, PEB


// ntdll imports (undocumented but stable)
using NtQueryInformationProcessFn = NTSTATUS(NTAPI *)(HANDLE, PROCESSINFOCLASS,
                                                      PVOID, ULONG, PULONG);

namespace Core {

// ─── Internal helpers ───────────────────────────────────────────────────────

static NtQueryInformationProcessFn s_ntQip = nullptr;

static void EnsureNtQip() {
  if (!s_ntQip) {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    s_ntQip = reinterpret_cast<NtQueryInformationProcessFn>(
        GetProcAddress(ntdll, "NtQueryInformationProcess"));
  }
}

// ─── SpoofProcessName ────────────────────────────────────────────────────────
// Overwrites ImagePathName and CommandLine in the current process's PEB
// so that NtQueryInformationProcess / Process Hacker / VAC see a fake name.
void Stealth::SpoofProcessName(const wchar_t *targetName) {
  EnsureNtQip();
  if (!s_ntQip)
    return;

  PROCESS_BASIC_INFORMATION pbi{};
  ULONG retLen = 0;
  if (s_ntQip(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi),
              &retLen) != 0)
    return;

  PEB *peb = pbi.PebBaseAddress;
  if (!peb || !peb->ProcessParameters)
    return;

  RTL_USER_PROCESS_PARAMETERS *params = peb->ProcessParameters;

  // Build the fake wide path once
  static wchar_t fakePath[MAX_PATH];
  static wchar_t fakeCmd[MAX_PATH];
  static bool built = false;
  if (!built) {
    wchar_t sysDir[MAX_PATH];
    GetSystemDirectoryW(sysDir, MAX_PATH);
    // e.g. "C:\Windows\System32\nvcontainer.exe"
    swprintf_s(fakePath, MAX_PATH, L"%s\\%s", sysDir, targetName);
    swprintf_s(fakeCmd, MAX_PATH, L"\"%s\"", fakePath);
    built = true;
  }

  const size_t pathLen = wcslen(fakePath) * sizeof(wchar_t);
  const size_t cmdLen = wcslen(fakeCmd) * sizeof(wchar_t);

  // Overwrite in-place (buffers are already allocated by the OS loader)
  if (params->ImagePathName.Buffer &&
      params->ImagePathName.MaximumLength >= (USHORT)pathLen) {
    memcpy(params->ImagePathName.Buffer, fakePath, pathLen);
    params->ImagePathName.Length = (USHORT)pathLen;
  }
  if (params->CommandLine.Buffer &&
      params->CommandLine.MaximumLength >= (USHORT)cmdLen) {
    memcpy(params->CommandLine.Buffer, fakeCmd, cmdLen);
    params->CommandLine.Length = (USHORT)cmdLen;
  }
}

// ─── Apply ───────────────────────────────────────────────────────────────────
void Stealth::Apply() {
  // Spoof as NVIDIA container process — exactly what nvcontainer.exe looks like
  SpoofProcessName(L"nvcontainer.exe");
}

} // namespace Core
