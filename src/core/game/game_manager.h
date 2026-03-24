#pragma once
#include "../sdk/entity.h"
#include "../sdk/structs.h"
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <vector>

namespace Core {
class GameManager {
public:
  static bool Init();
  static void Update();

  static void EnableBoneRead(bool enable);
  static void EnableWeaponRead(bool enable);

  // Rendering getters (thread-safe using shared_mutex)
  static SDK::Matrix4x4 GetViewMatrix();
  static uintptr_t GetClientBase();
  static std::vector<SDK::Entity> GetRenderPlayers();

  // Local player accessors (used by radar, features)
  static SDK::Vector3 GetLocalPos();
  static SDK::Vector3 GetLocalEyePos();
  static SDK::Vector2 GetLocalAngles(); // Use .y for yaw
  static SDK::Vector2 GetLocalAimPunch();
  static int GetLocalShotsFired();
  static int GetLocalTeam();
  static bool IsLocalScoped();
  static uint32_t GetLocalCrosshairEntityHandle();

  // Engine pointers for external feature logic
  static uintptr_t GetLocalPlayerPawn();
  static uintptr_t GetEntityList();
  // NOTE: Makes a live RPM call — call only from memory thread, not render thread
  static uintptr_t GetEntityFromHandle(uint32_t handle);

  // Weapon and entity data accessors (to avoid MemoryManager reads in features)
  static std::string GetLocalWeaponName();
  static uintptr_t GetEntityGameSceneNode(uintptr_t entity);

  // Bomb State
  static SDK::BombInfo GetBombInfo();

private:
  static uintptr_t clientBase;
  static std::atomic<bool> s_readBones;
  static std::atomic<bool> s_readWeapons;

  // -- Backend State (Written by Memory Thread) --
  static SDK::Matrix4x4 viewMatrix;
  static std::vector<SDK::Entity> players;
  static SDK::Vector3 localPos;
  static SDK::Vector3 localEyePos;
  static SDK::Vector2 localAngles;
  static SDK::Vector2 localAimPunch;
  static int localShotsFired;
  static int localTeam;
  static bool localScoped;
  static uint32_t localCrosshairHandle;
  static uintptr_t localPawn;
  static uintptr_t entityList;
  static SDK::BombInfo bombInfo;

  // -- Frontend State (Thread-Safe Cached Copies for Render) --
  static std::vector<SDK::Entity> playerBuffers[2];
  static std::atomic<int> activeBufferIndex;

  static std::shared_mutex stateMutex;
  static SDK::Matrix4x4 cachedViewMatrix;
  static SDK::Vector3 cachedLocalPos;
  static SDK::Vector3 cachedLocalEyePos;
  static SDK::Vector2 cachedLocalAngles;
  static SDK::Vector2 cachedLocalAimPunch;
  static int cachedLocalShotsFired;
  static int cachedLocalTeam;
  static bool cachedLocalScoped;
  static uint32_t cachedLocalCrosshairHandle;
  static uintptr_t cachedLocalPawn;
  static uintptr_t cachedEntityList;
  static SDK::BombInfo cachedBombInfo;
};
} // namespace Core
