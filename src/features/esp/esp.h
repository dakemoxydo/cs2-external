#pragma once
#include "esp_config.h"
#include "features/feature_base.h"

namespace Features {
extern EspConfig config;

class Esp : public IFeature {
public:
  void Update() override;
  void Render(Render::DrawList &drawList) override;
  const char *GetName() override { return "ESP"; }
};
} // namespace Features
