#pragma once
#include "features/feature_base.h"

namespace Features {

class Esp : public IFeature {
public:
  void Update() override;
  void Render(Render::DrawList &drawList) override;
  std::string_view GetName() override { return "ESP"; }
  void RenderUI() override;
};
} // namespace Features
