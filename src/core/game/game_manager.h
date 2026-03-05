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
  static float GetLocalYaw();
  static int GetLocalTeam();

private:
  static uintptr_t clientBase;

  // -- Backend State (Written by Memory Thread) --
  static SDK::Matrix4x4 viewMatrix;
  static std::vector<SDK::Entity> players;
  static SDK::Vector3 localPos;
  static float localYaw;
  static int localTeam;

  // -- Frontend State (Thread-Safe Copies for Render) --
  static std::shared_mutex stateMutex;
  static SDK::Matrix4x4 renderViewMatrix;
  static std::vector<SDK::Entity> renderPlayers;
  static SDK::Vector3 renderLocalPos;
  static float renderLocalYaw;
  static int renderLocalTeam;
};
} // namespace Core
