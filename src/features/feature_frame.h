#pragma once

#include "config/settings.h"
#include "core/game/game_snapshot.h"

namespace Features {

struct FeatureFrame {
  const Core::GameSnapshot &game;
  const Config::GlobalSettings &settings;
};

} // namespace Features
