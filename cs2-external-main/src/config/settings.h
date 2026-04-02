#pragma once
#include "features/aimbot/aimbot_config.h"
#include "features/bomb/bomb_config.h"
#include "features/debug_overlay/debug_overlay_config.h"
#include "features/esp/esp_config.h"
#include "features/misc/misc_config.h"
#include "features/radar/radar_config.h"
#include "features/triggerbot/triggerbot_config.h"
#include "features/rcs/rcs_config.h"
#include "features/skinchanger/skinchanger_config.h"

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
  Features::SkinchangerConfig skinchanger;

  PerformanceConfig performance; // Added performance settings
  bool menuIsOpen = false;
};

extern GlobalSettings Settings;
} // namespace Config
