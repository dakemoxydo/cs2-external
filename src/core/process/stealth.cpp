#include "stealth.h"
#include <cstring>
#include <cwchar>
#include <winternl.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <string>
#include <random>

namespace Core {

// ─── Internal helpers ───────────────────────────────────────────────────────

// NtSetInformationProcess for PEB spoofing
using NtSetInformationProcessFn = NTSTATUS(NTAPI *)(HANDLE, PROCESSINFOCLASS,
                                                    PVOID, ULONG);

// NtQueryInformationProcess for debugger detection
using NtQueryInformationProcessFn = NTSTATUS(NTAPI *)(HANDLE, PROCESSINFOCLASS,
                                                      PVOID, ULONG, PULONG);

static NtSetInformationProcessFn s_ntSip = nullptr;
static NtQueryInformationProcessFn s_ntQip = nullptr;

static void EnsureNtFunctions() {
  HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
  if (!ntdll)
    return;

  if (!s_ntSip) {
    s_ntSip = reinterpret_cast<NtSetInformationProcessFn>(
        GetProcAddress(ntdll, "NtSetInformationProcess"));
  }
  if (!s_ntQip) {
    s_ntQip = reinterpret_cast<NtQueryInformationProcessFn>(
        GetProcAddress(ntdll, "NtQueryInformationProcess"));
  }
}

// ─── PEB Process Name Spoofing ───────────────────────────────────────────────
// Overwrites ImagePathName in RTL_USER_PROCESS_PARAMETERS to disguise the
// process name visible via NtQueryInformationProcess / WinAPI name queries.

static void SpoofProcessName() {
  if (!s_ntQip)
    return;

  PROCESS_BASIC_INFORMATION pbi = {};
  ULONG len = 0;
  if (s_ntQip(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi),
              &len) != 0)
    return;

  // PEB.ProcessParameters → RTL_USER_PROCESS_PARAMETERS
  // Both offsets are stable on x64 Windows 10/11:
  //   PEB.ProcessParameters = PEB + 0x20
  //   RTL_USER_PROCESS_PARAMETERS.ImagePathName = params + 0x60
  //   RTL_USER_PROCESS_PARAMETERS.CommandLine    = params + 0x70

  PEB *peb = reinterpret_cast<PEB *>(pbi.PebBaseAddress);
  if (!peb)
    return;

  RTL_USER_PROCESS_PARAMETERS *params = peb->ProcessParameters;
  if (!params)
    return;

  // Build a spoofed name that looks like a legitimate system process
  // Use a static wide buffer — lives for the process lifetime (no dangling ptr)
  static wchar_t spoofedName[] = L"explorer.exe";
  static wchar_t spoofedCmdLine[] = L"C:\\Windows\\explorer.exe";

  UNICODE_STRING newImagePath;
  newImagePath.Buffer = spoofedName;
  newImagePath.Length =
      static_cast<USHORT>(wcslen(spoofedName) * sizeof(wchar_t));
  newImagePath.MaximumLength = newImagePath.Length + sizeof(wchar_t);

  UNICODE_STRING newCmdLine;
  newCmdLine.Buffer = spoofedCmdLine;
  newCmdLine.Length =
      static_cast<USHORT>(wcslen(spoofedCmdLine) * sizeof(wchar_t));
  newCmdLine.MaximumLength = newCmdLine.Length + sizeof(wchar_t);

  // Write directly to the PEB fields (same process, no VirtualProtect needed
  // for writable data segment of process parameters)
  params->ImagePathName = newImagePath;
  params->CommandLine = newCmdLine;
}

// ─── Anti-Debugger Detection ─────────────────────────────────────────────────

static bool IsBeingDebugged() {
  // 1. Check PEB.BeingDebugged flag via NtQueryInformationProcess
  if (s_ntQip) {
    PROCESS_BASIC_INFORMATION pbi = {};
    ULONG len = 0;
    if (s_ntQip(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi),
                &len) == 0 &&
        pbi.PebBaseAddress != nullptr) {
      PEB *peb = reinterpret_cast<PEB *>(pbi.PebBaseAddress);
      if (peb->BeingDebugged)
        return true;
    }
  }

  // 2. Fallback: IsDebuggerPresent (easily patched, but fast)
  if (IsDebuggerPresent())
    return true;

  // 3. Check for debug port (ProcessDebugPort = 0x07)
  HANDLE hDebugPort = nullptr;
  if (s_ntQip && s_ntQip(GetCurrentProcess(),
                          static_cast<PROCESSINFOCLASS>(0x07),
                          &hDebugPort, sizeof(hDebugPort), nullptr) == 0) {
    if (hDebugPort != nullptr)
      return true;
  }

  return false;
}

// ─── Thread Hiding ───────────────────────────────────────────────────────────

static void HideCurrentThread() {
  // Set thread information to hide from debugger
  // Use NtSetInformationThread with ThreadHideFromDebugger
  typedef NTSTATUS(NTAPI *NtSetInformationThreadFn)(HANDLE, THREADINFOCLASS,
                                                    PVOID, ULONG);
  HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
  if (!ntdll)
    return;

  auto NtSetInformationThread = reinterpret_cast<NtSetInformationThreadFn>(
      GetProcAddress(ntdll, "NtSetInformationThread"));
  if (NtSetInformationThread) {
    // ThreadHideFromDebugger = 0x11
    NtSetInformationThread(GetCurrentThread(), (THREADINFOCLASS)0x11, NULL, 0);
  }
}

// ─── Memory Operation Randomization ──────────────────────────────────────────

static std::mt19937_64 s_randGen;
static std::uniform_int_distribution<> s_sleepDist(0, 2);

static void RandomizeTiming() {
  // Add random micro-sleeps to break timing patterns
  int sleepMs = s_sleepDist(s_randGen);
  if (sleepMs > 0) {
    Sleep(sleepMs);
  }
}

// ─── Apply Stealth Measures ──────────────────────────────────────────────────

void Stealth::Apply() {
  EnsureNtFunctions();

  // Hide current thread from debugger
  HideCurrentThread();

  // Seed random generator
  std::random_device rd;
  s_randGen.seed(rd());

  // Check for debugger and exit if detected
  if (IsBeingDebugged()) {
    ExitProcess(0);
  }

  // Spoof process name in PEB
  SpoofProcessName();
}

// ─── Randomized Sleep Helper ─────────────────────────────────────────────────

void Stealth::RandomizedSleep(int baseMs, int varianceMs) {
  if (varianceMs <= 0) {
    Sleep(baseMs);
    return;
  }

  std::uniform_int_distribution<> dist(-varianceMs, varianceMs);
  int actualSleep = baseMs + dist(s_randGen);
  if (actualSleep < 1)
    actualSleep = 1;

  Sleep(actualSleep);
}

// ─── Check if process should exit (panic trigger) ───────────────────────────

bool Stealth::ShouldPanic() {
  // Check for common analysis tools
  const wchar_t* blacklisted[] = {
      L"procmon.exe",
      L"procmon64.exe",
      L"ollydbg.exe",
      L"x64dbg.exe",
      L"cheatengine-x86_64.exe",
      L"ProcessHacker.exe",
      nullptr
  };

  HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hSnap == INVALID_HANDLE_VALUE)
    return false;

  PROCESSENTRY32W entry{.dwSize = sizeof(entry)};
  if (Process32FirstW(hSnap, &entry)) {
    do {
      for (int i = 0; blacklisted[i]; i++) {
        if (_wcsicmp(entry.szExeFile, blacklisted[i]) == 0) {
          CloseHandle(hSnap);
          return true;
        }
      }
    } while (Process32NextW(hSnap, &entry));
  }
  CloseHandle(hSnap);
  return false;
}

} // namespace Core
