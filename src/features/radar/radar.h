#pragma once
#include "features/feature_base.h"

namespace Features {
class Radar : public IFeature {
public:
  void Update() override;
  void Render(Render::DrawList &drawList) override;
  const char *GetName() override { return "Radar"; }
  void RenderUI() override;
};
} // namespace Features
