#include "aimbot.h"
#include "aimbot_config.h"
#include "../../config/settings.h"
#include "../../core/game/game_manager.h"
#include "../../core/math/math.h"
#include "../../input/input_manager.h"
#include "../../core/process/stealth.h"
#include "../rcs/rcs.h"
#include "../feature_base.h"
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
static uintptr_t s_lastTarget = 0;
static float s_dxRemainder = 0.0f;
static float s_dyRemainder = 0.0f;

// Pre-computed jitter table for small micro-movements (16 steps)
static const float s_jitterTable[] = {
    0.01f, -0.02f, 0.03f, 0.01f, -0.01f, 0.02f, -0.03f, 0.01f,
    0.02f, -0.01f, 0.01f, -0.02f, 0.01f, 0.03f, -0.01f, 0.02f
};
static int s_jitterIdx = 0;

// Thread-local RNG for human-like noise
static thread_local std::mt19937 s_rng(std::random_device{}());
static thread_local std::uniform_real_distribution<float> s_noiseDist(-0.5f, 0.5f);
static thread_local std::uniform_int_distribution<int> s_microDelayDist(0, 9);

// ── Mouse movement using mouse_event (less detectable than SendInput)
static void SendMouse(float dpitch, float dyaw, float sensitivity) {
  // COUNTS_PER_DEG depends on user's in-game sensitivity setting
  const float sens = fmaxf(0.001f, sensitivity);
  const float countsPerDeg = 1.0f / (0.022f * sens);

  float fx = -dyaw * countsPerDeg + s_dxRemainder;
  float fy = dpitch * countsPerDeg + s_dyRemainder;

  // Near target noise reduction to eliminate jitter
  float totalMove = sqrtf(fx * fx + fy * fy);
  if (totalMove > 0.4f) {
      float noiseScale = fminf(1.0f, totalMove * 0.15f);
      fx += s_noiseDist(s_rng) * 0.25f * noiseScale;
      fy += s_noiseDist(s_rng) * 0.25f * noiseScale;
  }

  int dx = static_cast<int>(fx);
  int dy = static_cast<int>(fy);

  // Store remainder for next frame with sub-pixel precision
  s_dxRemainder = fx - static_cast<float>(dx);
  s_dyRemainder = fy - static_cast<float>(dy);

  if (dx != 0 || dy != 0) {
      Input::InputManager::SendMouseDelta(dx, dy);
  }

  // Micro-delay to break timing patterns
  if (s_microDelayDist(s_rng) == 0) {
    Core::Stealth::RandomizedSleep(0, 1);
  }
}

std::string_view Aimbot::GetName() { return "Aimbot"; }

void Aimbot::Update() {
  // Snapshot settings once for consistency and future thread-safety
  struct S {
    bool enabled, teamCheck, onlyScoped, targetLock, visibleOnly;
    int hotkey, targetBone;
    float fov, smooth, jitter, sensitivity;
    bool rcsEnabled;
    float rcsPitch, rcsYaw;
    int rcsStartBullet;
  };
  S s;
  {
    std::shared_lock<std::shared_mutex> lock(Config::SettingsMutex);
    auto &A = Config::Settings.aimbot;
    auto &R = Config::Settings.rcs;
    s = {A.enabled, A.teamCheck, A.onlyScoped, A.targetLock, A.visibleOnly,
         A.hotkey, A.targetBone, A.fov, A.smooth, A.jitter, A.sensitivity,
         R.enabled, R.pitchStrength, R.yawStrength, R.startBullet};
  }

  if (!s.enabled) { s_lastTarget = 0; return; }
  if (Render::Menu::IsOpen()) { s_lastTarget = 0; return; }
  if (!Input::InputManager::IsKeyDown(s.hotkey)) { s_lastTarget = 0; return; }
  if (s.onlyScoped && !Core::GameManager::IsLocalScoped()) { s_lastTarget = 0; return; }

  const auto snapshot = Core::GameManager::GetSnapshot();
  SDK::Vector3 eyePos = snapshot->localEyePos;
  SDK::Vector2 eyeAngles = snapshot->localAngles;
  SDK::Vector2 aimPunch = snapshot->localAimPunch;
  int shotsFired = snapshot->localShotsFired;
  const std::string &weapon = snapshot->localWeaponName;

  bool rcsActive = RCSSystem::IsWeaponSupported(weapon) && shotsFired >= s.rcsStartBullet;

  const auto &players = snapshot->players;
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
      if (s_lastTarget != 0 && p.address != s_lastTarget && s.targetLock) continue;
      if (p.bonePositions.empty() || s.targetBone >= (int)p.bonePositions.size()) continue;

      SDK::Vector3 bonePos = p.bonePositions[s.targetBone];
      SDK::Vector2 targetAngles = Core::Math::CalcAngle(eyePos, bonePos);

      if (rcsActive && s.rcsEnabled) {
        targetAngles.x -= aimPunch.x * s.rcsPitch * 2.0f;
        targetAngles.y -= aimPunch.y * s.rcsYaw * 2.0f;
      }

      float dp = Core::Math::DeltaAngle(eyeAngles.x, targetAngles.x);
      float dy = Core::Math::DeltaAngle(eyeAngles.y, targetAngles.y);
      float delta = sqrtf(dp * dp + dy * dy);
      if (delta < bestAngleDelta) { bestAngleDelta = delta; bestTarget = &p; }
    }

    if (!bestTarget && s.targetLock && s_lastTarget != 0) {
      s_lastTarget = 0;
      retryCount++;
      if (retryCount <= MAX_RETRIES) { retryWithoutLock = true; continue; }
    }
    if (!bestTarget) return;

    static float s_lastTotalDelta = 0.0f;
    static ULONGLONG s_pauseUntil = 0;

    SDK::Vector3 targetPos = bestTarget->bonePositions[s.targetBone];
    float jBase = s_jitterTable[s_jitterIdx & 15] * s.jitter * 40.0f;
    s_jitterIdx = (s_jitterIdx + 1) & 15;
    targetPos.z += jBase;

    SDK::Vector2 finalTargetAngles = Core::Math::CalcAngle(eyePos, targetPos);
    if (rcsActive && s.rcsEnabled) {
      finalTargetAngles.x -= aimPunch.x * s.rcsPitch * 2.0f;
      finalTargetAngles.y -= aimPunch.y * s.rcsYaw * 2.0f;
    }

    float dp = Core::Math::DeltaAngle(eyeAngles.x, finalTargetAngles.x);
    float dyw = Core::Math::DeltaAngle(eyeAngles.y, finalTargetAngles.y);
    float totalDelta = sqrtf(dp * dp + dyw * dyw);

    if (s_lastTarget == bestTarget->address) {
      if (totalDelta > s_lastTotalDelta + 1.0f) s_pauseUntil = GetTickCount64() + 350;
    }
    s_lastTotalDelta = totalDelta;
    if (GetTickCount64() < s_pauseUntil) { s_lastTarget = bestTarget->address; return; }

    float baseSmooth = fmaxf(1.0f, s.smooth);
    float step = totalDelta / baseSmooth;
    if (totalDelta < 0.15f) { step *= 0.2f; }
    else { float minVel = 0.35f / baseSmooth; if (step < minVel) step = minVel; }
    if (step > totalDelta) step = totalDelta;
    float ratio = (totalDelta > 0.001f) ? (step / totalDelta) : 1.0f;

    SendMouse(dp * ratio, dyw * ratio, s.sensitivity);
    s_lastTarget = bestTarget->address;
  } while (retryWithoutLock);
}

void Aimbot::Render(Render::DrawList &drawList) {
  // Snapshot settings for render
  bool isEnabled, showFov;
  float fov;
  {
    std::shared_lock<std::shared_mutex> lock(Config::SettingsMutex);
    isEnabled = Config::Settings.aimbot.enabled;
    showFov = Config::Settings.aimbot.showFov;
    fov = Config::Settings.aimbot.fov;
  }
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
