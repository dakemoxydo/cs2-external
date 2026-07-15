#pragma once
#include "features/aimbot/aimbot_config.h"
#include "features/bomb/bomb_config.h"
#include "features/chams/chams_config.h"
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
#include <type_traits>

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
  Features::ChamsConfig chams;
  Features::DebugConfig debug;
  Features::RCSConfig rcs;
  Features::SoundEspConfig soundEsp;

  PerformanceConfig performance; // Added performance settings
};

extern GlobalSettings Settings;
extern std::shared_mutex SettingsMutex;

namespace Detail {
// Applies lifecycle changes after a settings write has committed. Passing an
// immutable copy keeps lifecycle hooks from observing mutable global state.
void ApplySettings(const GlobalSettings& snapshot);
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
  using Result = std::invoke_result_t<Fn &, GlobalSettings &>;
  static_assert(!std::is_void_v<Result>, "Use MutateSettingsVoid for void callbacks");
  Result result{};
  GlobalSettings snapshot;
  {
    std::unique_lock<std::shared_mutex> lock(SettingsMutex);
    result = fn(Settings);
    snapshot = Settings;
  }
  Detail::ApplySettings(snapshot);
  return result;
}

template <typename Fn>
void MutateSettingsVoid(Fn &&fn) {
  GlobalSettings snapshot;
  {
    std::unique_lock<std::shared_mutex> lock(SettingsMutex);
    fn(Settings);
    snapshot = Settings;
  }
  Detail::ApplySettings(snapshot);
}

} // namespace Config
