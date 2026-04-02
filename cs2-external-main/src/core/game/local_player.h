#pragma once
#include "core/sdk/player.h"

namespace Core {
class LocalPlayer {
public:
  static void Update();
  static SDK::Player *Get();

private:
  static std::unique_ptr<SDK::Player> player;
};
} // namespace Core
