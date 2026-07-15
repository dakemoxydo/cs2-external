#pragma once
#include "config/settings.h"
#include "bomb_config.h"
#include "features/feature_base.h"
#include "render/draw/draw_list.h"


namespace Features {

class Bomb : public IFeature {
public:
  std::string_view GetName() override { return "BombTimer"; }
  void Update(const FeatureFrame &frame) override;
  void Render(const FeatureFrame &frame, Render::DrawList &drawList) override;
  void OnDisable() override;
  void RenderUI() override;

private:
  void ResetState();

  bool planted_ = false;
  float timeLeft_ = 0.0f;
  float totalTime_ = 40.0f;
  bool isBeingDefused_ = false;
  float defuseTimeLeft_ = 0.0f;
  int site_ = -1;
};

} // namespace Features


