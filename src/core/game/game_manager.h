#pragma once
#include "../sdk/entity.h"
#include "../sdk/structs.h"
#include <vector>

namespace Core {
class GameManager {
public:
  static bool Init();
  static void Update();

  static SDK::Matrix4x4 GetViewMatrix();
  static uintptr_t GetClientBase();
  static const std::vector<SDK::Entity> &GetPlayers();

private:
  static SDK::Matrix4x4 viewMatrix;
  static uintptr_t clientBase;
  static std::vector<SDK::Entity> players;
};
} // namespace Core
