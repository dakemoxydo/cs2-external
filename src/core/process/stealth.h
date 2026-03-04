#pragma once
#include <windows.h>

namespace Core {

/// Stealth: user-mode anti-detection routines.
/// Call Stealth::Apply() BEFORE Process::Attach() at program startup.
class Stealth {
public:
  /// Rename our own process in the PEB so tools/VAC see a fake module path.
  /// targetName: e.g. L"nvcontainer.exe"
  static void SpoofProcessName(const wchar_t* targetName);

  /// Apply all stealth measures at once.
  static void Apply();
};

} // namespace Core
