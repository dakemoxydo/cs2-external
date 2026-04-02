#pragma once
#include "features/feature_base.h"

namespace Features {
class Triggerbot : public IFeature {
public:
  void Update() override;
  void Render(Render::DrawList &drawList) override;
  const char *GetName() override { return "Triggerbot"; }
  void RenderUI() override;
};
} // namespace Features
