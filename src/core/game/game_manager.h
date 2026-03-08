#pragma once
#include "../sdk/entity.h"
#include "../sdk/structs.h"
#include <mutex>
#include <shared_mutex>
#include <vector>

namespace Core {
class GameManager {
public:
  static bool Init();
  static void Update();

  // Rendering getters (thread-safe using shared_mutex)
  static SDK::Matrix4x4 GetViewMatrix();
  static uintptr_t GetClientBase();
  static std::vector<SDK::Entity> GetRenderPlayers();

  // Local player accessors (used by radar, features)
  static SDK::Vector3 GetLocalPos();
  static float GetLocalYaw(); // Deprecated? Only radar uses it directly or we
                              // keep it. It returns angles.y
  static SDK::Vector2 GetLocalAngles();
  static int GetLocalTeam();
  static bool IsLocalScoped();
  static uint32_t GetLocalCrosshairEntityHandle();

  // Engine pointers for external feature logic
  static uintptr_t GetLocalPlayerPawn();
  static uintptr_t GetEntityList();
  static uintptr_t GetEntityFromHandle(uint32_t handle);

  // Weapon and entity data accessors (to avoid MemoryManager reads in features)
  static std::string GetLocalWeaponName();
  static uintptr_t GetWeaponServices(uintptr_t pawn);
  static uint32_t GetActiveWeaponHandle(uintptr_t weaponServices);
  static short GetEntityItemDefinitionIndex(uintptr_t entity);
  static uintptr_t GetEntityGameSceneNode(uintptr_t entity);
  static uint64_t GetModelHandle(uintptr_t gameSceneNode);
  static uint64_t FindModelHandleByDefIndex(int defIndex);

  // Bomb State
  static SDK::BombInfo GetBombInfo();

private:
  static uintptr_t clientBase;

  // -- Backend State (Written by Memory Thread) --
  static SDK::Matrix4x4 viewMatrix;
  static std::vector<SDK::Entity> players;
  static SDK::Vector3 localPos;
  static SDK::Vector2 localAngles;
  static float localYaw;
  static int localTeam;
  static bool localScoped;
  static uint32_t localCrosshairHandle;
  static uintptr_t localPawn;
  static uintptr_t entityList;
  static SDK::BombInfo bombInfo;

  // -- Frontend State (Thread-Safe Copies for Render) --
  static std::shared_mutex stateMutex;
  static SDK::Matrix4x4 renderViewMatrix;
  static std::vector<SDK::Entity> renderPlayers;
  static SDK::Vector3 renderLocalPos;
  static SDK::Vector2 renderLocalAngles;
  static float renderLocalYaw; // kept for backwards compat with radar
  static int renderLocalTeam;
  static bool renderLocalScoped;
  static uint32_t renderLocalCrosshairHandle;
  static uintptr_t renderLocalPawn;
  static uintptr_t renderEntityList;
  static SDK::BombInfo renderBombInfo;
};
} // namespace Core
