#pragma once
#include "../feature_base.h"

namespace Features {
class Aimbot : public IFeature {
public:
  void Update() override;
  void Render(Render::DrawList &drawList) override;
  std::string_view GetName() override;
  void RenderUI() override;
};
} // namespace Features
