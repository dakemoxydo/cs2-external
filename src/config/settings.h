#pragma once
#include "features/aimbot/aimbot_config.h"
#include "features/bomb/bomb_config.h"
#include "features/debug_overlay/debug_overlay_config.h"
#include "features/esp/esp_config.h"
#include "features/misc/misc_config.h"
#include "features/radar/radar_config.h"
#include "features/triggerbot/triggerbot_config.h"
#include "features/rcs/rcs_config.h"
#include "features/sound_esp/sound_esp_config.h"
#include <mutex>
#include <shared_mutex>
#include <utility>

namespace Config {

struct PerformanceConfig {
  int fpsLimit = 240;
  int upsLimit = 240; // Updates per second for memory reading
  bool vsyncEnabled = false;
};

struct GlobalSettings {
  Features::EspConfig esp;
  Features::AimbotConfig aimbot;
  Features::TriggerbotConfig triggerbot;
  Features::RadarConfig radar;
  Features::MiscConfig misc;
  Features::BombConfig bomb;
  Features::DebugConfig debug;
  Features::RCSConfig rcs;
  Features::SoundEspConfig soundEsp;

  PerformanceConfig performance; // Added performance settings
};

extern GlobalSettings Settings;
extern std::shared_mutex SettingsMutex;

namespace Detail {
void ApplySettingsUnderLock();
}

inline GlobalSettings CopySettings() {
  std::shared_lock<std::shared_mutex> lock(SettingsMutex);
  return Settings;
}

// ── Thread-safe settings read ─────────────────────────────────────────
// All feature Update()/Render() functions must read settings under
// shared_lock to prevent data races with Load()/Save()/UI writes.
//
// Usage:
//   Config::ReadSettings([](const auto &S) {
//       bool enabled = S.esp.enabled;
//       ...
//   });
template <typename Fn>
auto ReadSettings(Fn &&fn) {
  std::shared_lock<std::shared_mutex> lock(SettingsMutex);
  return fn(Settings);
}

template <typename Fn>
auto MutateSettings(Fn &&fn) {
  std::unique_lock<std::shared_mutex> lock(SettingsMutex);
  auto result = fn(Settings);
  Detail::ApplySettingsUnderLock();
  return result;
}

template <typename Fn>
void MutateSettingsVoid(Fn &&fn) {
  std::unique_lock<std::shared_mutex> lock(SettingsMutex);
  fn(Settings);
  Detail::ApplySettingsUnderLock();
}

} // namespace Config
