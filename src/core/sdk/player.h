#pragma once
#include "entity.h"
#include <string>

namespace SDK {
class Player : public Entity {
public:
  Player(uintptr_t address);

  std::string GetName() const;
  Vector3 GetBonePosition(int boneId) const;
  int GetArmor() const;
};
} // namespace SDK
