#pragma once
#include "config/settings.h"
#include "bomb_config.h"
#include "features/feature_base.h"
#include "render/draw/draw_list.h"


namespace Features {

extern BombConfig bombConfig;

class Bomb : public IFeature {
public:
  std::string_view GetName() override { return "BombTimer"; }
  void Update() override;
  void Render(Render::DrawList &drawList) override;
  void RenderUI() override;
};

} // namespace Features


