#pragma once
#include "core/sdk/structs.h"

namespace Render {
class WorldToScreen {
public:
  static bool Convert(const SDK::Vector3 &world, SDK::Vector2 &screen,
                      const SDK::Matrix4x4 &viewMatrix, int screenWidth,
                      int screenHeight);
};
} // namespace Render
