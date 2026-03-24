#pragma once
#include <windows.h>

namespace Core {

/// Stealth: user-mode anti-detection routines.
/// Call Stealth::Apply() BEFORE Process::Attach() at program startup.
class Stealth {
public:
  /// Apply all stealth measures at once.
  static void Apply();

  /// Randomized sleep to break timing patterns.
  static void RandomizedSleep(int baseMs, int varianceMs);

  /// Check if analysis tools are running (panic trigger).
  static bool ShouldPanic();
};

} // namespace Core
