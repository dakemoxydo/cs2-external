#include "math.h"
#include <cmath>

static constexpr float M_PI_F = 3.14159265f;
static constexpr float RAD2DEG = 180.0f / M_PI_F;
static constexpr float DEG2RAD = M_PI_F / 180.0f;

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

// Compute pitch/yaw angles from src to dst (degrees)
SDK::Vector2 Math::CalcAngle(const SDK::Vector3 &src, const SDK::Vector3 &dst) {
  float dx = dst.x - src.x;
  float dy = dst.y - src.y;
  float dz = dst.z - src.z;
  float dist2d = sqrtf(dx * dx + dy * dy);

  SDK::Vector2 angles;
  angles.x = -atan2f(dz, dist2d) * RAD2DEG; // pitch
  angles.y = atan2f(dy, dx) * RAD2DEG;      // yaw
  return angles;
}

// Normalized angle delta in [-180, 180]
float Math::DeltaAngle(float a, float b) {
  float d = fmodf(b - a + 180.0f, 360.0f) - 180.0f;
  return d < -180.0f ? d + 360.0f : d;
}

// Clamp angle to [-180, 180]
float Math::ClampAngle(float a) {
  while (a > 180.0f)
    a -= 360.0f;
  while (a < -180.0f)
    a += 360.0f;
  return a;
}

float Math::Lerp(float a, float b, float t) { return a + (b - a) * t; }

// Sigmoid-derived velocity factor: fast start, slow approach
// Returns t in (0,1) for x in [0,∞); higher x = further from target = faster
float Math::Sigmoid(float x) {
  // Derivative of sigmoid at x — peaks near x=0, giving deceleration
  float s = 1.0f / (1.0f + expf(-x));
  return s * (1.0f - s) * 4.0f; // normalized to ~1.0 peak
}

} // namespace Core
