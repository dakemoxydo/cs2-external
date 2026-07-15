#include "aimbot.h"
#include "aimbot_config.h"
#include "../../config/settings.h"
#include "../../core/game/game_manager.h"
#include "../../core/math/math.h"
#include "../../input/input_manager.h"
#include "../../core/process/stealth.h"
#include "../feature_base.h"
#include "../feature_frame.h"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <random>
#include <imgui.h>
#include "../../render/draw/draw_list.h"
#include "../../render/overlay/overlay.h"
#include "../../render/menu/menu.h"
#include <shared_mutex>

namespace Features {

// ── Internal state (Safe to use static as all features update in one thread)

// Pre-computed jitter table for small micro-movements (16 steps)
static constexpr float kJitterTable[] = {
    0.01f, -0.02f, 0.03f, 0.01f, -0.01f, 0.02f, -0.03f, 0.01f,
    0.02f, -0.01f, 0.01f, -0.02f, 0.01f, 0.03f, -0.01f, 0.02f
};

// ── Mouse movement using mouse_event (less detectable than SendInput)
void Aimbot::SendMouse(float dpitch, float dyaw, float sensitivity) {
  // COUNTS_PER_DEG depends on user's in-game sensitivity setting
  const float sens = fmaxf(0.001f, sensitivity);
  const float countsPerDeg = 1.0f / (0.022f * sens);

  float fx = -dyaw * countsPerDeg + dxRemainder_;
  float fy = dpitch * countsPerDeg + dyRemainder_;

  // Near target noise reduction to eliminate jitter
  float totalMove = sqrtf(fx * fx + fy * fy);
  if (totalMove > 0.4f) {
      float noiseScale = fminf(1.0f, totalMove * 0.15f);
      fx += noiseDist_(rng_) * 0.25f * noiseScale;
      fy += noiseDist_(rng_) * 0.25f * noiseScale;
  }

  int dx = static_cast<int>(fx);
  int dy = static_cast<int>(fy);

  // Store remainder for next frame with sub-pixel precision
  dxRemainder_ = fx - static_cast<float>(dx);
  dyRemainder_ = fy - static_cast<float>(dy);

  if (dx != 0 || dy != 0) {
      Input::InputManager::SendMouseDelta(dx, dy);
  }

  // Micro-delay to break timing patterns
  if (microDelayDist_(rng_) == 0) {
    Core::Stealth::RandomizedSleep(0, 1);
  }
}

void Aimbot::ResetState() {
  lastTarget_ = 0;
  dxRemainder_ = 0.0f;
  dyRemainder_ = 0.0f;
  jitterIndex_ = 0;
  lastTotalDelta_ = 0.0f;
  pauseUntil_ = 0;
}

void Aimbot::OnDisable() { ResetState(); }

std::string_view Aimbot::GetName() { return "Aimbot"; }

void Aimbot::Update(const FeatureFrame &frame) {
  // Snapshot settings once for consistency and future thread-safety
  struct S {
    bool enabled, teamCheck, onlyScoped, targetLock, visibleOnly;
    int hotkey, targetBone;
    float fov, smooth, jitter, sensitivity;
  };
  S s;
  {
    const auto &A = frame.settings.aimbot;
    s = {A.enabled, A.teamCheck, A.onlyScoped, A.targetLock, A.visibleOnly,
         A.hotkey, A.targetBone, A.fov, A.smooth, A.jitter, A.sensitivity};
  }

  if (!s.enabled) { ResetState(); return; }
  if (Render::Menu::IsOpen()) { ResetState(); return; }
  if (!Input::InputManager::IsKeyDown(s.hotkey)) { ResetState(); return; }
  if (s.onlyScoped && !frame.game.localScoped) { ResetState(); return; }

  const auto &snapshot = frame.game;
  SDK::Vector3 eyePos = snapshot.localEyePos;
  SDK::Vector2 eyeAngles = snapshot.localAngles;

  const auto &players = snapshot.players;
  if (players.empty()) return;

  const SDK::Entity *bestTarget = nullptr;
  float bestAngleDelta = s.fov;

  bool retryWithoutLock = false;
  int retryCount = 0;
  constexpr int MAX_RETRIES = 2;

  do {
    retryWithoutLock = false;
    for (const auto &p : players) {
      if (p.health <= 0 || p.health > 100) continue;
      if (s.teamCheck && p.isTeammate) continue;
      if (s.visibleOnly && !p.isSpotted) continue;
      if (lastTarget_ != 0 && p.address != lastTarget_ && s.targetLock) continue;
      if (p.bonePositions.empty() || s.targetBone >= (int)p.bonePositions.size()) continue;

      SDK::Vector3 bonePos = p.bonePositions[s.targetBone];
      SDK::Vector2 targetAngles = Core::Math::CalcAngle(eyePos, bonePos);

      float dp = Core::Math::DeltaAngle(eyeAngles.x, targetAngles.x);
      float dy = Core::Math::DeltaAngle(eyeAngles.y, targetAngles.y);
      float delta = sqrtf(dp * dp + dy * dy);
      if (delta < bestAngleDelta) { bestAngleDelta = delta; bestTarget = &p; }
    }

    if (!bestTarget && s.targetLock && lastTarget_ != 0) {
      lastTarget_ = 0;
      retryCount++;
      if (retryCount <= MAX_RETRIES) { retryWithoutLock = true; continue; }
    }
    if (!bestTarget) return;

    SDK::Vector3 targetPos = bestTarget->bonePositions[s.targetBone];
    float jBase = kJitterTable[jitterIndex_ & 15] * s.jitter * 40.0f;
    jitterIndex_ = (jitterIndex_ + 1) & 15;
    targetPos.z += jBase;

    SDK::Vector2 finalTargetAngles = Core::Math::CalcAngle(eyePos, targetPos);

    float dp = Core::Math::DeltaAngle(eyeAngles.x, finalTargetAngles.x);
    float dyw = Core::Math::DeltaAngle(eyeAngles.y, finalTargetAngles.y);
    float totalDelta = sqrtf(dp * dp + dyw * dyw);

    if (lastTarget_ == bestTarget->address) {
      if (totalDelta > lastTotalDelta_ + 1.0f) pauseUntil_ = GetTickCount64() + 350;
    }
    lastTotalDelta_ = totalDelta;
    if (GetTickCount64() < pauseUntil_) { lastTarget_ = bestTarget->address; return; }

    float baseSmooth = fmaxf(1.0f, s.smooth);
    float step = totalDelta / baseSmooth;
    if (totalDelta < 0.15f) { step *= 0.2f; }
    else { float minVel = 0.35f / baseSmooth; if (step < minVel) step = minVel; }
    if (step > totalDelta) step = totalDelta;
    float ratio = (totalDelta > 0.001f) ? (step / totalDelta) : 1.0f;

    SendMouse(dp * ratio, dyw * ratio, s.sensitivity);
    lastTarget_ = bestTarget->address;
  } while (retryWithoutLock);
}

void Aimbot::Render(const FeatureFrame &frame, Render::DrawList &drawList) {
  // Snapshot settings for render
  bool isEnabled, showFov;
  float fov;
  isEnabled = frame.settings.aimbot.enabled;
  showFov = frame.settings.aimbot.showFov;
  fov = frame.settings.aimbot.fov;
  if (!isEnabled || !showFov) return;

  int gameW = Render::Overlay::GetGameWidth();
  int gameH = Render::Overlay::GetGameHeight();
  if (gameW <= 0 || gameH <= 0) return;

  float cx = gameW * 0.5f;
  float cy = gameH * 0.5f;
  constexpr float CS2_HFOV = 106.0f;
  float radiusPx = (fov / CS2_HFOV) * gameW;
  float circleCol[4] = {1.0f, 1.0f, 1.0f, 0.35f};
  drawList.DrawCircle(cx, cy, radiusPx, circleCol, 64, 1.0f);
}

void Aimbot::RenderUI() {}

} // namespace Features
