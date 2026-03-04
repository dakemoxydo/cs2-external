#include "process.h"
#include <iostream>
#include <tlhelp32.h>
#include <vector>
#include <winternl.h>


// NtQuerySystemInformation (undocumented but stable ntdll export)
using NtQsiFunc = NTSTATUS(NTAPI *)(ULONG, PVOID, ULONG, PULONG);

// Our own struct to avoid conflicting with winternl typedefs
namespace {
struct Cs2HandleEntry {
  ULONG OwnerPid;
  BYTE ObjectTypeIndex;
  BYTE Flags;
  USHORT HandleValue;
  PVOID Object;
  ACCESS_MASK GrantedAccess;
};
struct Cs2HandleInfo {
  ULONG Count;
  Cs2HandleEntry Entries[1];
};
} // anonymous namespace

namespace Core {

HANDLE Process::hProcess = nullptr;
DWORD Process::processId = 0;
NtReadVirtualMemoryFn Process::s_ntRvm = nullptr;

// ─── ResolveNtFunctions ─────────────────────────────────────────────────────
void Process::ResolveNtFunctions() {
  HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
  if (!ntdll)
    return;
  s_ntRvm = reinterpret_cast<NtReadVirtualMemoryFn>(
      GetProcAddress(ntdll, "NtReadVirtualMemory"));
}

// ─── NtRead ─────────────────────────────────────────────────────────────────
bool Process::NtRead(void *address, void *buffer, size_t size) {
  if (!s_ntRvm || !hProcess)
    return false;
  SIZE_T read = 0;
  return s_ntRvm(hProcess, address, buffer, (SIZE_T)size, &read) == 0 &&
         read == size;
}

// ─── TryStealHandle ─────────────────────────────────────────────────────────
// Enumerate all handles in the system; look for one pointing to cs2.exe (by
// PID) that is owned by a trusted process, then DuplicateHandle it into our
// process.
HANDLE Process::TryStealHandle(DWORD targetPid) {
  static NtQsiFunc ntqsi = nullptr;
  if (!ntqsi) {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    ntqsi = reinterpret_cast<NtQsiFunc>(
        GetProcAddress(ntdll, "NtQuerySystemInformation"));
  }
  if (!ntqsi)
    return nullptr;

  // Grow buffer until NtQsi fits
  ULONG bufSize = 1 << 21; // 2 MB start
  std::vector<BYTE> buf;
  NTSTATUS st;
  do {
    buf.resize(bufSize);
    ULONG retLen = 0;
    st = ntqsi(16 /*SystemHandleInformation*/, buf.data(), bufSize, &retLen);
    bufSize <<= 1;
  } while (st == (NTSTATUS)0xC0000004L); // STATUS_INFO_LENGTH_MISMATCH
  if (st < 0)
    return nullptr;

  auto *info = reinterpret_cast<Cs2HandleInfo *>(buf.data());

  // Trusted process names whose handles we would steal
  const wchar_t *trusted[] = {L"steam.exe", L"steamwebhelper.exe",
                              L"gameoverlayrenderer64.exe", L"nvcontainer.exe",
                              nullptr};

  for (ULONG i = 0; i < info->Count; i++) {
    Cs2HandleEntry &e = info->Entries[i];

    if (!(e.GrantedAccess & PROCESS_VM_READ))
      continue;
    if (e.OwnerPid == GetCurrentProcessId())
      continue;
    if (e.OwnerPid == 0 || e.OwnerPid == 4)
      continue;

    HANDLE hOwner =
        OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_LIMITED_INFORMATION,
                    FALSE, e.OwnerPid);
    if (!hOwner)
      continue;

    wchar_t name[MAX_PATH] = {};
    DWORD len = MAX_PATH;
    QueryFullProcessImageNameW(hOwner, 0, name, &len);

    bool ok = false;
    for (int t = 0; trusted[t]; t++)
      if (wcsstr(name, trusted[t])) {
        ok = true;
        break;
      }

    if (!ok) {
      CloseHandle(hOwner);
      continue;
    }

    HANDLE dup = nullptr;
    if (!DuplicateHandle(
            hOwner, reinterpret_cast<HANDLE>((uintptr_t)e.HandleValue),
            GetCurrentProcess(), &dup,
            PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, 0)) {
      CloseHandle(hOwner);
      continue;
    }
    CloseHandle(hOwner);

    if (::GetProcessId(dup) == targetPid) {
      std::cout << "[INFO] Handle stolen from trusted PID " << e.OwnerPid
                << "\n";
      return dup;
    }
    CloseHandle(dup);
  }
  return nullptr;
}

// ─── Attach ─────────────────────────────────────────────────────────────────
bool Process::Attach(const std::wstring &processName) {
  ResolveNtFunctions();

  HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hSnap == INVALID_HANDLE_VALUE)
    return false;

  PROCESSENTRY32W entry{.dwSize = sizeof(entry)};
  bool found = false;
  if (Process32FirstW(hSnap, &entry)) {
    do {
      if (!_wcsicmp(entry.szExeFile, processName.c_str())) {
        processId = entry.th32ProcessID;
        found = true;
        break;
      }
    } while (Process32NextW(hSnap, &entry));
  }
  CloseHandle(hSnap);
  if (!found)
    return false;

  // Try handle theft first (stealthier)
  hProcess = TryStealHandle(processId);

  // Fallback: regular OpenProcess
  if (!hProcess) {
    std::cout << "[INFO] Handle steal failed — using OpenProcess (fallback)\n";
    hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION,
                           FALSE, processId);
  }

  if (!hProcess) {
    processId = 0;
    return false;
  }
  return true;
}

// ─── Detach ─────────────────────────────────────────────────────────────────
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
