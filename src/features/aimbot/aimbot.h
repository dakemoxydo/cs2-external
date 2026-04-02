#pragma once
#include "../feature_base.h"

namespace Features {
class Aimbot : public IFeature {
public:
  void Update() override;
  void Render(Render::DrawList &drawList) override;
  const char *GetName() override;
  void RenderUI() override;
};
} // namespace Features
