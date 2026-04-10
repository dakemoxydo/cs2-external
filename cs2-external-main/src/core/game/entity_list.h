#pragma once
#include "core/sdk/player.h"
#include <memory>
#include <vector>

namespace Core {
class EntityList {
public:
  static void Update();
  static const std::vector<std::shared_ptr<SDK::Player>> &GetPlayers();

private:
  static std::vector<std::shared_ptr<SDK::Player>> players;
};
} // namespace Core
