#include "process.h"
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

bool IsHandleAlive(HANDLE handle) {
  if (!handle) {
    return false;
  }

  DWORD exitCode = 0;
  return GetExitCodeProcess(handle, &exitCode) != 0 && exitCode == STILL_ACTIVE;
}
} // anonymous namespace

namespace Core {

std::mutex Process::s_mutex;
HANDLE Process::hProcess = nullptr;
std::atomic<DWORD> Process::processId{0};
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
NTSTATUS Process::NtRead(void *address, void *buffer, size_t size) {
  if (!s_ntRvm)
    return (NTSTATUS)0xC000000DL;

  // Duplicate the handle to protect against Detach() closing it concurrently.
  // Without this, there's a use-after-free: we copy hProcess under lock,
  // release the lock, then another thread calls Detach() which closes the
  // handle before we reach s_ntRvm below.
  HANDLE h = nullptr;
  {
    std::lock_guard lock(s_mutex);
    if (!hProcess)
      return (NTSTATUS)0xC000000DL;
    if (!DuplicateHandle(GetCurrentProcess(), hProcess, GetCurrentProcess(),
                         &h, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
      return (NTSTATUS)0xC000000DL;
    }
  }

  SIZE_T read = 0;
  NTSTATUS status = s_ntRvm(h, address, buffer, (SIZE_T)size, &read);
  CloseHandle(h);

  if (status != 0 || read != size) {
    // Read failed — expected behavior for invalid entity pointers
  }
  return status;
}

// ─── NtWrite ─────────────────────────────────────────────────────────────────
bool Process::NtWrite(void *address, const void *buffer, size_t size) {
  // Duplicate the handle to protect against Detach() closing it concurrently.
  HANDLE h = nullptr;
  {
    std::lock_guard lock(s_mutex);
    if (!hProcess)
      return false;
    if (!DuplicateHandle(GetCurrentProcess(), hProcess, GetCurrentProcess(),
                         &h, PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, 0)) {
      return false;
    }
  }

  SIZE_T written = 0;
  BOOL ok = WriteProcessMemory(h, address, buffer, size, &written);
  CloseHandle(h);
  return ok && written == size;
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

  PROCESSENTRY32W entry{};
  entry.dwSize = sizeof(PROCESSENTRY32W);
  bool found = false;
  if (Process32FirstW(hSnap, &entry)) {
    do {
      if (!_wcsicmp(entry.szExeFile, processName.c_str())) {
        {
          std::lock_guard lock(s_mutex);
          processId = entry.th32ProcessID;
        }
        found = true;
        break;
      }
    } while (Process32NextW(hSnap, &entry));
  }
  CloseHandle(hSnap);
  if (!found) {
    Detach();
    return false;
  }

  // Try handle theft first (stealthier)
  HANDLE stolen = TryStealHandle(entry.th32ProcessID);

  // Fallback: regular OpenProcess
  if (!stolen) {
    stolen = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION,
                           FALSE, entry.th32ProcessID);
  }

  if (!stolen) {
    {
      std::lock_guard lock(s_mutex);
      processId = 0;
    }
    return false;
  }

  {
    std::lock_guard lock(s_mutex);
    if (hProcess && hProcess != stolen) {
      CloseHandle(hProcess);
    }
    hProcess = stolen;
    processId = entry.th32ProcessID;
  }
  return true;
}

// ─── Detach ─────────────────────────────────────────────────────────────────
void Process::Detach() {
  std::lock_guard lock(s_mutex);
  if (hProcess) {
    CloseHandle(hProcess);
    hProcess = nullptr;
  }
  processId = 0;
}

DWORD Process::GetProcessId() {
  std::lock_guard lock(s_mutex);

  if (hProcess && !IsHandleAlive(hProcess)) {
    CloseHandle(hProcess);
    hProcess = nullptr;
    processId = 0;
  }

  return processId.load();
}

} // namespace Core
