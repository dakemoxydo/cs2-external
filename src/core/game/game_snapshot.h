#pragma once

#include "core/sdk/entity.h"
#include "core/sdk/structs.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Core {

// Immutable frame data shared from the memory thread to rendering/features.
struct GameSnapshot {
  uintptr_t clientBase = 0;
  uintptr_t entityList = 0;
  SDK::Matrix4x4 viewMatrix = {};
  std::vector<SDK::Entity> players;
  SDK::Vector3 localPos = {};
  SDK::Vector3 localEyePos = {};
  SDK::Vector2 localAngles = {};
  SDK::Vector2 localShootAngle = {};
  SDK::Vector2 localAimPunch = {};
  int localShotsFired = 0;
  int localTeam = 0;
  bool localScoped = false;
  uint32_t localCrosshairHandle = 0;
  uintptr_t localPawn = 0;
  SDK::BombInfo bombInfo = {};
  std::string localWeaponName;
  float localWeaponRange = 0.0f;
  std::vector<SDK::BulletImpactInfo> localBulletImpacts;
  float frameTimeSeconds = 0.0f;
  std::vector<SDK::ShotEvent> shotEvents;
  std::vector<SDK::BulletTraceEvent> bulletTraceEvents;
  std::vector<SDK::HitEvent> hitEvents;
  std::vector<SDK::MovementAudioEvent> movementAudioEvents;
};

} // namespace Core
