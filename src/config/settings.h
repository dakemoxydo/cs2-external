#pragma once
#include "features/aimbot/aimbot_config.h"
#include "features/esp/esp_config.h"
#include "features/misc/misc_config.h"
#include "features/radar/radar_config.h"
#include "features/triggerbot/triggerbot_config.h"


namespace Config {
struct GlobalSettings {
  Features::EspConfig esp;
  Features::AimbotConfig aimbot;
  Features::TriggerbotConfig triggerbot;
  Features::RadarConfig radar;
  Features::MiscConfig misc;
};

extern GlobalSettings Settings;
} // namespace Config
