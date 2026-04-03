#pragma once
#include "features/feature_base.h"

namespace Features {
class Misc : public IFeature {
public:
  void Update() override;
  void Render(Render::DrawList &drawList) override;
  std::string_view GetName() override { return "Misc"; }
  void RenderUI() override;
};
} // namespace Features
