#pragma once
#include "features/feature_base.h"

namespace Features {
class Radar : public IFeature {
public:
  void Update() override;
  void Render(Render::DrawList &drawList) override;
  std::string_view GetName() override { return "Radar"; }
  void RenderUI() override;
};
} // namespace Features
