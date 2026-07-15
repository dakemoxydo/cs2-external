#include "game_manager.h"
#include <memory>

namespace Core {

std::shared_ptr<const GameSnapshot> GameManager::GetSnapshot() {
  auto snapshot = s_stateStore.Load();
  if (snapshot) {
    return snapshot;
  }

  static const std::shared_ptr<const GameSnapshot> emptySnapshot =
      std::make_shared<const GameSnapshot>();
  return emptySnapshot;
}

SDK::Matrix4x4 GameManager::GetViewMatrix() {
  return GetSnapshot()->viewMatrix;
}

std::vector<SDK::Entity> GameManager::GetRenderPlayers() {
  return GetSnapshot()->players;
}

SDK::Vector3 GameManager::GetLocalPos() {
  return GetSnapshot()->localPos;
}

SDK::Vector3 GameManager::GetLocalEyePos() {
  return GetSnapshot()->localEyePos;
}

SDK::Vector2 GameManager::GetLocalAimPunch() {
  return GetSnapshot()->localAimPunch;
}

SDK::Vector2 GameManager::GetLocalShootAngle() {
  return GetSnapshot()->localShootAngle;
}

int GameManager::GetLocalShotsFired() {
  return GetSnapshot()->localShotsFired;
}

int GameManager::GetLocalTeam() {
  return GetSnapshot()->localTeam;
}

SDK::Vector2 GameManager::GetLocalAngles() {
  return GetSnapshot()->localAngles;
}

bool GameManager::IsLocalScoped() {
  return GetSnapshot()->localScoped;
}

uint32_t GameManager::GetLocalCrosshairEntityHandle() {
  return GetSnapshot()->localCrosshairHandle;
}

SDK::BombInfo GameManager::GetBombInfo() {
  return GetSnapshot()->bombInfo;
}

uintptr_t GameManager::GetLocalPlayerPawn() {
  return GetSnapshot()->localPawn;
}

uintptr_t GameManager::GetEntityList() {
  return GetSnapshot()->entityList;
}

uintptr_t GameManager::GetClientBase() {
  return GetSnapshot()->clientBase;
}

std::string GameManager::GetLocalWeaponName() {
  return GetSnapshot()->localWeaponName;
}

float GameManager::GetLocalWeaponRange() {
  return GetSnapshot()->localWeaponRange;
}

} // namespace Core
