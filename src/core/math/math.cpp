#include "math.h"

namespace Core {
bool Math::WorldToScreen(const SDK::Vector3 &pos, SDK::Vector2 &out,
                         const SDK::Matrix4x4 &matrix, int screenWidth,
                         int screenHeight) {

  float clipW = matrix.m[3][0] * pos.x + matrix.m[3][1] * pos.y +
                matrix.m[3][2] * pos.z + matrix.m[3][3];

  if (clipW < 0.001f)
    return false;

  float clipX = matrix.m[0][0] * pos.x + matrix.m[0][1] * pos.y +
                matrix.m[0][2] * pos.z + matrix.m[0][3];
  float clipY = matrix.m[1][0] * pos.x + matrix.m[1][1] * pos.y +
                matrix.m[1][2] * pos.z + matrix.m[1][3];

  out.x = (screenWidth / 2.0f) * (1.0f + clipX / clipW);
  out.y = (screenHeight / 2.0f) * (1.0f - clipY / clipW);

  return true;
}
} // namespace Core
