#pragma once
#include "../sdk/structs.h"

namespace Core {
class Math {
public:
  // Render
  static bool WorldToScreen(const SDK::Vector3 &pos, SDK::Vector2 &out,
                            const SDK::Matrix4x4 &matrix, int screenWidth,
                            int screenHeight);

  // Aimbot math utilities
  static SDK::Vector2 CalcAngle(const SDK::Vector3 &src,
                                const SDK::Vector3 &dst);
  static float DeltaAngle(float a,
                          float b); // normalized difference in [-180, 180]
  static float ClampAngle(float a); // clamp to [-180, 180]
  static float Lerp(float a, float b, float t);
  static float Sigmoid(float x); // smooth curve for human-like aiming
};
} // namespace Core
