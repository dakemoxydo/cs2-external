#pragma once
#include "features/feature_base.h"

namespace Features {
class BunnyHop : public IFeature {
public:
  void Update() override;
  void Render(Render::DrawList &drawList) override;
  const char *GetName() override { return "BunnyHop"; }
};
} // namespace Features
