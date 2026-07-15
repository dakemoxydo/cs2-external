#pragma once
#include "config/settings.h"
#include "debug_overlay_config.h"
#include "features/feature_base.h"
#include "render/draw/draw_list.h"
#include <chrono>


namespace Features {

class DebugOverlay : public IFeature {
public:
  std::string_view GetName() override { return "DebugOverlay"; }

  void Update(const FeatureFrame &frame) override;
  void Render(const FeatureFrame &frame, Render::DrawList &drawList) override;
  void OnEnable() override;
  void RenderUI() override;

private:
  std::chrono::steady_clock::time_point lastFrame_ =
      std::chrono::steady_clock::now();
  float fps_ = 0.0f;
};

} // namespace Features


