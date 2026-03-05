#include "features/feature_manager.h"
#include "features/aimbot/aimbot.h"
#include "features/bomb/bomb.h"
#include "features/debug_overlay/debug_overlay.h"
#include "features/esp/esp.h"
#include "features/misc/misc.h"
#include "features/radar/radar.h"
#include "features/triggerbot/triggerbot.h"
#include <memory>
#include <string>
#include <vector>


namespace Features {
std::vector<std::unique_ptr<IFeature>> FeatureManager::features;

void FeatureManager::RegisterAll() {
  features.push_back(std::make_unique<Esp>());
  features.push_back(std::make_unique<Aimbot>());
  features.push_back(std::make_unique<Triggerbot>());
  features.push_back(std::make_unique<Misc>());
  features.push_back(std::make_unique<Bomb>());
  features.push_back(std::make_unique<Radar>());
  features.push_back(std::make_unique<DebugOverlay>());
}

void FeatureManager::UpdateAll() {
  for (auto &feature : features)
    if (feature->IsEnabled())
      feature->Update();
}

void FeatureManager::RenderAll(Render::DrawList &drawList) {
  for (auto &feature : features)
    if (feature->IsEnabled())
      feature->Render(drawList);
}

} // namespace Features
