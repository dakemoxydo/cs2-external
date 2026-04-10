#include "feature_manager.h"
#include "aimbot/aimbot.h"
#include "aimbot/aimbot_ui.h"
#include "bomb/bomb.h"
#include "bomb/bomb_ui.h"
#include "debug_overlay/debug_overlay.h"
#include "debug_overlay/debug_overlay_ui.h"
#include "esp/esp.h"
#include "esp/esp_ui.h"
#include "misc/misc.h"
#include "misc/misc_ui.h"
#include "radar/radar.h"
#include "radar/radar_ui.h"
#include "triggerbot/triggerbot.h"
#include "triggerbot/triggerbot_ui.h"
#include "rcs/rcs.h"
#include "rcs/rcs_ui.h"
#include "footsteps_esp/footsteps_esp.h"
#include "footsteps_esp/footsteps_esp_ui.h"
#include "feature_base.h"
#include <memory>
#include <string>
#include <vector>

namespace Features {
std::vector<FeatureManager::FeatureSlot> FeatureManager::featureSlots;

void FeatureManager::RegisterFeature(std::string_view name, FeatureFactory factory) {
  FeatureSlot slot(name, std::move(factory));
  featureSlots.push_back(std::move(slot));
}

void FeatureManager::RegisterAll() {
  RegisterFeature("ESP", []() { return std::make_unique<Esp>(); });
  RegisterFeature("Aimbot", []() { return std::make_unique<Aimbot>(); });
  RegisterFeature("Triggerbot", []() { return std::make_unique<Triggerbot>(); });
  RegisterFeature("Misc", []() { return std::make_unique<Misc>(); });
  RegisterFeature("Bomb", []() { return std::make_unique<Bomb>(); });
  RegisterFeature("Radar", []() { return std::make_unique<Radar>(); });
  RegisterFeature("DebugOverlay", []() { return std::make_unique<DebugOverlay>(); });
  RegisterFeature("RCSSystem", []() { return std::make_unique<RCSSystem>(); });
  RegisterFeature("FootstepsEsp", []() { return std::make_unique<FootstepsEsp>(); });
}

void FeatureManager::UpdateAll() {
  for (auto &slot : featureSlots) {
    if (!slot.instance) continue;
    if (slot.instance->IsEnabled())
      slot.instance->Update();
  }
}

void FeatureManager::RenderAll(Render::DrawList &drawList) {
  for (auto &slot : featureSlots) {
    if (!slot.instance) continue;
    if (slot.instance->IsEnabled())
      slot.instance->Render(drawList);
  }
}

void FeatureManager::EnsureFeatureInitialized(std::string_view name) {
  for (auto &slot : featureSlots) {
    if (slot.name == name && !slot.instance) {
      slot.instance = slot.factory();
      slot.instance->SetEnabled(true);
      return;
    }
    if (slot.name == name && slot.instance && !slot.instance->IsEnabled()) {
      slot.instance->SetEnabled(true);
      return;
    }
  }
}

void FeatureManager::EnsureAllInitialized() {
  for (auto &slot : featureSlots) {
    if (!slot.instance) {
      slot.instance = slot.factory();
    }
  }
}

IFeature* FeatureManager::GetFeature(std::string_view name) {
  for (auto &slot : featureSlots) {
    if (slot.name == name && slot.instance) {
      return slot.instance.get();
    }
  }
  return nullptr;
}

} // namespace Features
