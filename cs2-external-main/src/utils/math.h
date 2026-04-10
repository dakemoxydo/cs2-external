#pragma once
#include "core/sdk/structs.h"

namespace Utils {
class Math {
public:
  static float CalcDistance(const SDK::Vector3 &a, const SDK::Vector3 &b);
  static SDK::Vector2 CalcAngle(const SDK::Vector3 &src,
                                const SDK::Vector3 &dst);
  static void NormalizeAngle(SDK::Vector2 &angle);
  static float CalcFOV(const SDK::Vector2 &viewAngle,
                       const SDK::Vector2 &aimAngle);
};
} // namespace Utils
