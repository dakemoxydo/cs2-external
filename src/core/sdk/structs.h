#pragma once
#include <cstdint>

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

struct BulletImpactInfo {
  Vector3 position = {};
  float timestamp = 0.0f;
  float expireTime = 0.0f;
};

struct ShotEvent {
  uint64_t id = 0;
  int shotsFired = 0;
  Vector3 start = {};
  Vector2 angle = {};
  Vector3 predictedEnd = {};
  float timeSeconds = 0.0f;
};

struct BulletTraceEvent {
  uint64_t id = 0;
  uint64_t shotId = 0;
  Vector3 start = {};
  Vector3 end = {};
  bool confirmedImpact = false;
  float timeSeconds = 0.0f;
};

struct HitEvent {
  uint64_t id = 0;
  uint64_t shotId = 0;
  uintptr_t targetAddress = 0;
  Vector3 position = {};
  int oldHealth = 0;
  int newHealth = 0;
  float timeSeconds = 0.0f;
};

enum class MovementAudioType : int {
  Footstep = 0,
  Jump = 1,
  Land = 2,
};

struct MovementAudioEvent {
  uint64_t id = 0;
  uint32_t pawnHandle = 0;
  uintptr_t playerAddress = 0;
  Vector3 origin = {};
  MovementAudioType type = MovementAudioType::Footstep;
  float strength = 1.0f;
  bool isTeammate = false;
  float timeSeconds = 0.0f;
};
} // namespace SDK
