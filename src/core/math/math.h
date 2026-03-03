#pragma once
#include "../sdk/structs.h"

namespace Core {
class Math {
public:
  static bool WorldToScreen(const SDK::Vector3 &pos, SDK::Vector2 &out,
                            const SDK::Matrix4x4 &matrix, int screenWidth,
                            int screenHeight);
};
} // namespace Core
