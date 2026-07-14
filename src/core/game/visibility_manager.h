#pragma once

#include "../sdk/structs.h"
#include <cstdint>

namespace Core {

struct VisibilityResult {
  bool hasValue = false;
  bool visible = false;
};

class VisibilityManager {
public:
  static void PrepareFrame(float nowSeconds);
  static VisibilityResult QueryPlayerVisibility(uintptr_t localPawn,
                                                uintptr_t enemyPawn,
                                                const SDK::Vector3 &start,
                                                const SDK::Vector3 &end);
  static bool IsAvailable();
  static void ResetFrameState();
  static void ResetProcessState();
};

} // namespace Core
