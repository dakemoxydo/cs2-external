#include "triggerbot.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "core/memory/memory_manager.h"
#include "core/sdk/offsets.h"
#include "render/draw/draw_list.h"
#include "input/input_manager.h"
#include "triggerbot_config.h"
#include <chrono>
#include <random>
#include <windows.h>

namespace Features {

static std::mt19937& GetRng() {
  static std::mt19937 rng(static_cast<unsigned int>(
      std::chrono::steady_clock::now().time_since_epoch().count()));
  return rng;
}

static int RandRange(int lo, int hi) {
  if (lo >= hi)
    return lo;
  std::uniform_int_distribution<int> dist(lo, hi);
  return dist(GetRng());
}

static auto NowMs() { return std::chrono::steady_clock::now(); }
static long long ElapsedMs(std::chrono::steady_clock::time_point t) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(NowMs() - t)
      .count();
}

enum class TBState { IDLE, TARGET_FOUND, WAITING, SHOOTING, COOLDOWN };
static TBState s_state = TBState::IDLE;
static std::chrono::steady_clock::time_point s_timer;
static int s_delayMs = 0;

void Triggerbot::Update() {
  if (!Config::Settings.triggerbot.enabled) {
    s_state = TBState::IDLE;
    return;
  }

  bool keyHeld =
      (GetAsyncKeyState(Config::Settings.triggerbot.hotkey) & 0x8000) != 0;
  if (!keyHeld) {
    s_state = TBState::IDLE;
    return;
  }

  uint32_t crossHairHandle = Core::GameManager::GetLocalCrosshairEntityHandle();

  bool onEnemy = false;
  if (crossHairHandle != 0 && crossHairHandle != 0xFFFFFFFF) {
    for (const auto &p : Core::GameManager::GetRenderPlayers()) {
      if (!p.IsValid() || !p.IsAlive())
        continue;
      if (Config::Settings.triggerbot.teamCheck && p.isTeammate)
        continue;

      if (p.pawnHandle == crossHairHandle) {
        onEnemy = true;
        break;
      }
    }
  }

  switch (s_state) {
  case TBState::IDLE:
    if (onEnemy) {
      s_delayMs = RandRange(Config::Settings.triggerbot.delayMin,
                            Config::Settings.triggerbot.delayMax);
      s_timer = NowMs();
      s_state = TBState::TARGET_FOUND;
    }
    break;

  case TBState::TARGET_FOUND:
    if (!onEnemy) {
      s_state = TBState::IDLE;
      break;
    }
    s_state = TBState::WAITING;
    break;

  case TBState::WAITING:
    if (!onEnemy) {
      s_state = TBState::IDLE;
      break;
    }
    if (ElapsedMs(s_timer) >= s_delayMs) {
      Input::InputManager::SendMouseClick(true);
      s_timer = NowMs();
      s_state = TBState::SHOOTING;
    }
    break;

  case TBState::SHOOTING:
    if (ElapsedMs(s_timer) >= 25) {
      Input::InputManager::SendMouseClick(false);
      s_timer = NowMs();
      s_state = TBState::COOLDOWN;
    }
    break;

  case TBState::COOLDOWN:
    if (ElapsedMs(s_timer) >= RandRange(80, 200)) {
      s_state = TBState::IDLE;
    }
    break;
  }
}

void Triggerbot::Render(Render::DrawList &) {
}

} // namespace Features
