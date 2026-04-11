#include "core/constants.h"
#include "game_manager.h"
#include "../memory/memory_manager.h"
#include "../sdk/offsets.h"
#include <memory>

namespace Core {

std::shared_ptr<const GameSnapshot> GameManager::GetSnapshot() {
  auto snapshot = s_snapshot.load(std::memory_order_acquire);
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

uintptr_t GameManager::GetEntityFromHandle(uint32_t handle) {
  if (!handle || handle == 0xFFFFFFFF) {
    return 0;
  }

  const auto snapshot = GetSnapshot();
  if (!snapshot->entityList) {
    return 0;
  }

  uintptr_t listEntry = MemoryManager::Read<uintptr_t>(
      snapshot->entityList + Constants::ENTITY_LIST_HEADER_OFFSET +
      0x8 * ((handle & 0x7FFF) >> 9));
  if (!listEntry) {
    return 0;
  }

  return MemoryManager::Read<uintptr_t>(
      listEntry + Constants::ENTITY_IDENTITY_ENTRY_SIZE * (handle & 0x1FF));
}

std::string GameManager::GetLocalWeaponName() {
  return GetSnapshot()->localWeaponName;
}

uintptr_t GameManager::GetEntityGameSceneNode(uintptr_t entity) {
  if (!entity) {
    return 0;
  }

  const auto offsets = SDK::Offsets::GetCopy();
  return MemoryManager::Read<uintptr_t>(entity + offsets.m_pGameSceneNode);
}

} // namespace Core
