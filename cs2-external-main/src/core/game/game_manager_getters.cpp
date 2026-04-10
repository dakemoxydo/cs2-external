#include "core/constants.h"
#include "game_manager.h"
#include "../memory/memory_manager.h"
#include "../sdk/offsets.h"
#include <shared_mutex>

namespace Core {

// ── Thread-safe rendering getters ─────────────────────────────────────────────

SDK::Matrix4x4 GameManager::GetViewMatrix() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedViewMatrix;
}

std::vector<SDK::Entity> GameManager::GetRenderPlayers() {
  std::lock_guard<std::mutex> lock(bufferMutex);
  int readIdx = activeBufferIndex.load(std::memory_order_acquire);
  return playerBuffers[readIdx];
}

SDK::Vector3 GameManager::GetLocalPos() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedLocalPos;
}

SDK::Vector3 GameManager::GetLocalEyePos() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedLocalEyePos;
}

SDK::Vector2 GameManager::GetLocalAimPunch() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedLocalAimPunch;
}

int GameManager::GetLocalShotsFired() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedLocalShotsFired;
}

int GameManager::GetLocalTeam() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedLocalTeam;
}

SDK::Vector2 GameManager::GetLocalAngles() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedLocalAngles;
}

bool GameManager::IsLocalScoped() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedLocalScoped;
}

uint32_t GameManager::GetLocalCrosshairEntityHandle() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedLocalCrosshairHandle;
}

SDK::BombInfo GameManager::GetBombInfo() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedBombInfo;
}

uintptr_t GameManager::GetLocalPlayerPawn() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedLocalPawn;
}

uintptr_t GameManager::GetEntityList() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedEntityList;
}

// NOTE: Now protected by shared_lock — clientBase written once during Init()
uintptr_t GameManager::GetClientBase() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return clientBase;
}

// NOTE: Makes a live RPM call — must be called only from memory thread
uintptr_t GameManager::GetEntityFromHandle(uint32_t handle) {
  if (!handle || handle == 0xFFFFFFFF)
    return 0;

  uintptr_t list = GetEntityList();
  if (!list)
    return 0;

  uintptr_t listEntry = MemoryManager::Read<uintptr_t>(
      list + Constants::ENTITY_LIST_HEADER_OFFSET + 0x8 * ((handle & 0x7FFF) >> 9));
  if (!listEntry)
    return 0;

  uintptr_t entity =
      MemoryManager::Read<uintptr_t>(listEntry + Constants::ENTITY_IDENTITY_ENTRY_SIZE * (handle & 0x1FF));
  return entity;
}

std::string GameManager::GetLocalWeaponName() {
  uintptr_t pawn = GetLocalPlayerPawn();
  if (!pawn || pawn <= Constants::MIN_VALID_ADDRESS)
    return "";
  uintptr_t cw =
      MemoryManager::Read<uintptr_t>(pawn + SDK::Offsets::m_pClippingWeapon);
  if (cw <= Constants::MIN_VALID_ADDRESS)
    return "";
  uintptr_t wPtr = MemoryManager::Read<uintptr_t>(cw + 0x10);
  if (wPtr <= Constants::MIN_VALID_ADDRESS)
    return "";
  uintptr_t nPtr = MemoryManager::Read<uintptr_t>(wPtr + 0x20);
  if (nPtr <= Constants::MIN_VALID_ADDRESS)
    return "";
  char wb[64] = {};
  MemoryManager::ReadRaw(nPtr, wb, sizeof(wb) - 1);
  std::string name(wb);
  if (name.rfind("weapon_", 0) == 0)
    return name.substr(7);
  return name;
}

uintptr_t GameManager::GetEntityGameSceneNode(uintptr_t entity) {
  if (!entity)
    return 0;
  return MemoryManager::Read<uintptr_t>(entity +
                                        SDK::Offsets::m_pGameSceneNode);
}

} // namespace Core
