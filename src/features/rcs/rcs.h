#pragma once
#include "../feature_base.h"
#include "../../core/sdk/structs.h"
#include <string>

namespace Features {

class RCSSystem : public IFeature {
public:
  void Update() override;
  void Render(Render::DrawList &drawList) override;
  const char *GetName() override { return "RCSSystem"; }

  static bool IsWeaponSupported(const std::string& weaponName);
};

} // namespace Features
