#pragma once
#include "config/settings.h"
#include "debug_overlay_config.h"
#include "features/feature_base.h"
#include "render/draw/draw_list.h"


namespace Features {

extern DebugConfig debugConfig;

class DebugOverlay : public IFeature {
public:
  const char *GetName() override { return "DebugOverlay"; }

  void Update() override;
  void Render(Render::DrawList &drawList) override;
  void RenderUI() override;
};

} // namespace Features


