#include "features/feature_manager.h"
#include "features/bomb/bomb.h"
#include "features/debug_overlay/debug_overlay.h"
#include "features/esp/esp.h"
#include <memory>
#include <string>
#include <vector>

namespace Features {
std::vector<std::unique_ptr<IFeature>> FeatureManager::features;

void FeatureManager::RegisterAll() {
  features.push_back(std::make_unique<Esp>());
  features.push_back(std::make_unique<Bomb>());
  features.push_back(std::make_unique<DebugOverlay>());
  // Future features...
}

void FeatureManager::UpdateAll() {
  for (auto &feature : features) {
    if (feature->IsEnabled()) {
      feature->Update();
    }
  }
}

void FeatureManager::RenderAll(Render::DrawList &drawList) {
  for (auto &feature : features) {
    if (feature->IsEnabled()) {
      feature->Render(drawList);
    }
  }
}
} // namespace Features
