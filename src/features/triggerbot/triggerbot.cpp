#include "triggerbot.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "features/feature_frame.h"
#include "render/menu/menu.h"
#include "render/draw/draw_list.h"
#include "input/input_manager.h"
#include "triggerbot_config.h"
#include <chrono>
#include <random>
#include <shared_mutex>
#include <windows.h>

namespace Features {

int Triggerbot::RandRange(int lo, int hi) {
  if (lo >= hi)
    return lo;
  std::uniform_int_distribution<int> dist(lo, hi);
  return dist(rng_);
}

static auto NowMs() { return std::chrono::steady_clock::now(); }
static long long ElapsedMs(std::chrono::steady_clock::time_point t) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(NowMs() - t)
      .count();
}

void Triggerbot::ResetState() {
  if (state_ == State::Shooting) {
    Input::InputManager::SendMouseClick(false);
  }
  state_ = State::Idle;
  timer_ = {};
  delayMs_ = 0;
}

void Triggerbot::OnDisable() { ResetState(); }

void Triggerbot::Update(const FeatureFrame &frame) {
  // Snapshot triggerbot settings atomically
  struct S {
    bool enabled, teamCheck;
    int hotkey, delayMin, delayMax;
  };
  S s;
  {
    const auto &TB = frame.settings.triggerbot;
    s = {TB.enabled, TB.teamCheck, TB.hotkey, TB.delayMin, TB.delayMax};
  }

  if (!s.enabled) { ResetState(); return; }
  if (Render::Menu::IsOpen()) { ResetState(); return; }

  bool keyHeld = Input::InputManager::IsKeyDown(s.hotkey);
  if (!keyHeld) { ResetState(); return; }

  const auto &snapshot = frame.game;
  uint32_t crossHairHandle = snapshot.localCrosshairHandle;

  bool onEnemy = false;
  if (crossHairHandle != 0 && crossHairHandle != 0xFFFFFFFF) {
    for (const auto &p : snapshot.players) {
      if (!p.IsValid() || !p.IsAlive()) continue;
      if (s.teamCheck && p.isTeammate) continue;
      if (p.pawnHandle == crossHairHandle) { onEnemy = true; break; }
    }
  }

  switch (state_) {
  case State::Idle:
    if (onEnemy) {
      delayMs_ = RandRange(s.delayMin, s.delayMax);
      timer_ = NowMs();
      state_ = State::TargetFound;
    }
    break;
  case State::TargetFound:
    if (!onEnemy) { ResetState(); break; }
    state_ = State::Waiting;
    break;
  case State::Waiting:
    if (!onEnemy) { ResetState(); break; }
    if (ElapsedMs(timer_) >= delayMs_) {
      Input::InputManager::SendMouseClick(true);
      timer_ = NowMs();
      state_ = State::Shooting;
    }
    break;
  case State::Shooting:
    if (ElapsedMs(timer_) >= 25) {
      Input::InputManager::SendMouseClick(false);
      timer_ = NowMs();
      state_ = State::Cooldown;
    }
    break;
  case State::Cooldown:
    if (ElapsedMs(timer_) >= RandRange(80, 200)) { ResetState(); }
    break;
  }
}

void Triggerbot::Render(const FeatureFrame &, Render::DrawList &) {
}

void Triggerbot::RenderUI() {}

} // namespace Features
