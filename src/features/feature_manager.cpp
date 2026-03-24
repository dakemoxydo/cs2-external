#include "feature_manager.h"
#include "aimbot/aimbot.h"
#include "bomb/bomb.h"
#include "debug_overlay/debug_overlay.h"
#include "esp/esp.h"
#include "misc/misc.h"
#include "radar/radar.h"
#include "triggerbot/triggerbot.h"
#include "rcs/rcs.h"
#include "feature_base.h"
#include <memory>
#include <string>
#include <vector>

namespace Features {
std::vector<std::unique_ptr<IFeature>> FeatureManager::features;

void FeatureManager::RegisterAll() {
  auto add = [](std::unique_ptr<IFeature> f) {
    f->SetEnabled(true);
    features.push_back(std::move(f));
  };

  add(std::make_unique<Esp>());
  add(std::make_unique<Aimbot>());
  add(std::make_unique<Triggerbot>());
  add(std::make_unique<Misc>());
  add(std::make_unique<Bomb>());
  add(std::make_unique<Radar>());
  add(std::make_unique<DebugOverlay>());
  add(std::make_unique<RCSSystem>());
}

void FeatureManager::UpdateAll() {
  for (auto &feature : features) {
    if (feature->IsEnabled())
      feature->Update();
  }
}

void FeatureManager::RenderAll(Render::DrawList &drawList) {
  for (auto &feature : features) {
    if (feature->IsEnabled())
      feature->Render(drawList);
  }
}

} // namespace Features
