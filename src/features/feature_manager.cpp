#include "feature_manager.h"
#include "aimbot/aimbot.h"
#include "bomb/bomb.h"
#include "chams/chams.h"
#include "debug_overlay/debug_overlay.h"
#include "esp/esp.h"
#include "misc/misc.h"
#include "radar/radar.h"
#include "triggerbot/triggerbot.h"
#include "rcs/rcs.h"
#include "sound_esp/sound_esp.h"
#include "feature_base.h"
#include "utils/logger.h"
#include <exception>
#include <memory>
#include <string>
#include <vector>

namespace Features {
std::vector<FeatureManager::FeatureSlot> FeatureManager::featureSlots;

void FeatureManager::Quarantine(FeatureSlot &slot, std::string_view phase,
                                const char *message) {
  Utils::Logger::Error("Feature '%s' failed during %.*s%s%s; disabling it",
                       slot.name.c_str(), static_cast<int>(phase.size()),
                       phase.data(), message ? ": " : "", message ? message : "");
  slot.failed = true;
  if (slot.instance) {
    try {
      slot.instance->SetEnabled(false);
    } catch (...) {
      Utils::Logger::Error("Feature '%s' also failed during disable",
                           slot.name.c_str());
    }
    slot.instance.reset();
  }
}

bool FeatureManager::CreateInstance(FeatureSlot &slot) {
  if (slot.instance) return true;
  if (slot.failed) return false;
  try {
    slot.instance = slot.factory();
    if (!slot.instance) {
      Quarantine(slot, "factory", "factory returned null");
      return false;
    }
    return true;
  } catch (const std::exception &e) {
    Quarantine(slot, "factory", e.what());
  } catch (...) {
    Quarantine(slot, "factory", "unknown exception");
  }
  return false;
}

bool FeatureManager::SetSlotEnabled(FeatureSlot &slot, bool enabled) {
  if (!enabled) {
    // An explicit off transition clears quarantine and permits a later retry.
    slot.failed = false;
    if (!slot.instance) return true;
  } else if (!CreateInstance(slot)) {
    return false;
  }
  try {
    slot.instance->SetEnabled(enabled);
    return true;
  } catch (const std::exception &e) {
    Quarantine(slot, enabled ? "enable" : "disable", e.what());
  } catch (...) {
    Quarantine(slot, enabled ? "enable" : "disable", "unknown exception");
  }
  return false;
}

void FeatureManager::RegisterFeature(std::string_view name, FeatureFactory factory) {
  FeatureSlot slot(name, std::move(factory));
  featureSlots.push_back(std::move(slot));
}

void FeatureManager::RegisterAll() {
  if (!featureSlots.empty()) {
    return;
  }

  RegisterFeature("ESP", []() { return std::make_unique<Esp>(); });
  RegisterFeature("Chams", []() { return std::make_unique<Chams>(); });
  RegisterFeature("Aimbot", []() { return std::make_unique<Aimbot>(); });
  RegisterFeature("Triggerbot", []() { return std::make_unique<Triggerbot>(); });
  RegisterFeature("Misc", []() { return std::make_unique<Misc>(); });
  RegisterFeature("Bomb", []() { return std::make_unique<Bomb>(); });
  RegisterFeature("Radar", []() { return std::make_unique<Radar>(); });
  RegisterFeature("DebugOverlay", []() { return std::make_unique<DebugOverlay>(); });
  RegisterFeature("RCSSystem", []() { return std::make_unique<RCSSystem>(); });
  RegisterFeature("SoundEsp", []() { return std::make_unique<SoundEsp>(); });
}

void FeatureManager::UpdateAll(const FeatureFrame &frame) {
  for (auto &slot : featureSlots) {
    if (!slot.instance) continue;
    if (!slot.instance->IsEnabled()) continue;
    try {
      slot.instance->Update(frame);
    } catch (const std::exception &e) {
      Quarantine(slot, "update", e.what());
    } catch (...) {
      Quarantine(slot, "update", "unknown exception");
    }
  }
}

void FeatureManager::RenderAll(const FeatureFrame &frame,
                               Render::DrawList &drawList) {
  for (auto &slot : featureSlots) {
    if (!slot.instance) continue;
    if (!slot.instance->IsEnabled()) continue;
    try {
      slot.instance->Render(frame, drawList);
    } catch (const std::exception &e) {
      Quarantine(slot, "render", e.what());
    } catch (...) {
      Quarantine(slot, "render", "unknown exception");
    }
  }
}

void FeatureManager::EnsureFeatureInitialized(std::string_view name) {
  for (auto &slot : featureSlots) {
    if (slot.name == name) {
      SetSlotEnabled(slot, true);
      return;
    }
  }
}

void FeatureManager::EnsureAllInitialized() {
  for (auto &slot : featureSlots) {
    CreateInstance(slot);
  }
}

void FeatureManager::ShutdownAll() {
  for (auto &slot : featureSlots) {
    if (slot.instance && slot.instance->IsEnabled()) SetSlotEnabled(slot, false);
    slot.instance.reset();
    slot.failed = false;
  }
}

void FeatureManager::SetEnabled(std::string_view name, bool enabled) {
  for (auto &slot : featureSlots) {
    if (slot.name != name) {
      continue;
    }
    SetSlotEnabled(slot, enabled);
    return;
  }
}

void FeatureManager::ForEachInitialized(
    const std::function<void(IFeature &)> &visitor) {
  for (auto &slot : featureSlots) {
    if (!slot.instance) continue;
    try {
      visitor(*slot.instance);
    } catch (const std::exception &e) {
      Quarantine(slot, "visitor", e.what());
    } catch (...) {
      Quarantine(slot, "visitor", "unknown exception");
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
