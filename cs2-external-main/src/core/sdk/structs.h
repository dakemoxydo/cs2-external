#pragma once

namespace SDK {
struct Vector3 {
  float x, y, z;
};

struct Vector2 {
  float x, y;
};

struct Matrix4x4 {
  float m[4][4];
};

struct Color {
  int r, g, b, a;
};
} // namespace SDK
