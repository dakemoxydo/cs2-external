#include "stealth.h"
#include <cstring>
#include <cwchar>
#include <winternl.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <string>
#include <random>
#include <mutex>

namespace Core {

// NtSetInformationProcess for PEB spoofing
using NtSetInformationProcessFn = NTSTATUS(NTAPI *)(HANDLE, PROCESSINFOCLASS,
                                                    PVOID, ULONG);

// NtQueryInformationProcess for debugger detection
using NtQueryInformationProcessFn = NTSTATUS(NTAPI *)(HANDLE, PROCESSINFOCLASS,
                                                      PVOID, ULONG, PULONG);

static NtQueryInformationProcessFn s_ntQip = nullptr;

static void EnsureNtFunctions() {
  HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
  if (!ntdll)
    return;

  if (!s_ntQip) {
    s_ntQip = reinterpret_cast<NtQueryInformationProcessFn>(
        GetProcAddress(ntdll, "NtQueryInformationProcess"));
  }
}

static void SpoofProcessName() {
  if (!s_ntQip)
    return;

  PROCESS_BASIC_INFORMATION pbi = {};
  ULONG len = 0;
  if (s_ntQip(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi),
              &len) != 0)
    return;

  PEB *peb = reinterpret_cast<PEB *>(pbi.PebBaseAddress);
  if (!peb)
    return;

  RTL_USER_PROCESS_PARAMETERS *params = peb->ProcessParameters;
  if (!params)
    return;

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

  DWORD oldProtect = 0;
  SIZE_T regionSize = sizeof(UNICODE_STRING) * 2;
  if (VirtualProtect(&params->ImagePathName, regionSize, PAGE_READWRITE, &oldProtect)) {
    params->ImagePathName = newImagePath;
    params->CommandLine = newCmdLine;
    VirtualProtect(&params->ImagePathName, regionSize, oldProtect, &oldProtect);
  }
}

static bool IsBeingDebugged() {
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

  if (IsDebuggerPresent())
    return true;

  HANDLE hDebugPort = nullptr;
  if (s_ntQip && s_ntQip(GetCurrentProcess(),
                          static_cast<PROCESSINFOCLASS>(0x07),
                          &hDebugPort, sizeof(hDebugPort), nullptr) == 0) {
    if (hDebugPort != nullptr)
      return true;
  }

  return false;
}

static void HideCurrentThread() {
  typedef NTSTATUS(NTAPI *NtSetInformationThreadFn)(HANDLE, THREADINFOCLASS,
                                                    PVOID, ULONG);
  HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
  if (!ntdll)
    return;

  auto NtSetInformationThread = reinterpret_cast<NtSetInformationThreadFn>(
      GetProcAddress(ntdll, "NtSetInformationThread"));
  if (NtSetInformationThread) {
    NtSetInformationThread(GetCurrentThread(), (THREADINFOCLASS)0x11, NULL, 0);
  }
}

static std::mt19937_64 s_randGen;
static std::mutex s_rngMutex;

void Stealth::Apply() {
  EnsureNtFunctions();
  HideCurrentThread();

  std::random_device rd;
  {
    std::lock_guard<std::mutex> lock(s_rngMutex);
    s_randGen.seed(rd());
  }

  if (IsBeingDebugged()) {
    // ExitProcess(0); // Disabled for development
  }

  SpoofProcessName();
}

void Stealth::RandomizedSleep(int baseMs, int varianceMs) {
  if (varianceMs <= 0) {
    Sleep(baseMs);
    return;
  }

  std::uniform_int_distribution<> dist(-varianceMs, varianceMs);
  int actualSleep;
  {
    std::lock_guard<std::mutex> lock(s_rngMutex);
    actualSleep = baseMs + dist(s_randGen);
  }
  if (actualSleep < 1)
    actualSleep = 1;

  Sleep(actualSleep);
}

bool Stealth::ShouldPanic() {
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

  PROCESSENTRY32W entry{};
  entry.dwSize = sizeof(PROCESSENTRY32W);
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
