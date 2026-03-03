#pragma once
#include "structs.h"
#include <cstdint>
#include <string>

namespace SDK {
struct Entity {
  uintptr_t address = 0;
  uintptr_t controllerAddress = 0;
  int health = 0;
  int team = 0;
  bool isTeammate = false;
  Vector3 position = {0, 0, 0};
  std::string name;

  bool IsValid() const { return address != 0; }
  bool IsAlive() const { return health > 0 && health <= 100; }
};
} // namespace SDK
