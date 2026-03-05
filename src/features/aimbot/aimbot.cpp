#include "aimbot.h"
#include "aimbot_config.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "core/math/math.h"
#include "core/memory/memory_manager.h"
#include "core/process/process.h"
#include "core/sdk/offsets.h"
#include "render/draw/draw_list.h"
#include <cmath>
#include <cstdlib>
#include <imgui.h>
#include <windows.h>

namespace Features {

// ── Human-like aimbot constants
// ─────────────────────────────────────────────── Jitter table: 16 precomputed
// noise offsets (degrees) applied cyclically
static const float s_jitterTable[16] = {
    0.012f,  -0.018f, 0.007f, -0.031f, 0.022f, 0.011f,  -0.009f, 0.025f,
    -0.014f, 0.003f,  0.028f, -0.006f, 0.019f, -0.023f, 0.008f,  -0.017f,
};
static int s_jitterIdx = 0;

// ── Internal state
// ────────────────────────────────────────────────────────────
static uintptr_t s_lastTarget = 0;
static float s_dxRemainder = 0.0f;
static float s_dyRemainder = 0.0f;

// ── SendInput replaces deprecated mouse_event
// ─────────────────────────────────
static void SendMouse(float dpitch, float dyaw) {
  // COUNTS_PER_DEG depends on user's in-game sensitivity setting
  const float sens = Config::Settings.aimbot.sensitivity;
  const float countsPerDeg = 1.0f / (0.022f * sens);

  float fx = -dyaw * countsPerDeg + s_dxRemainder;
  float fy = dpitch * countsPerDeg + s_dyRemainder;

  int dx = static_cast<int>(fx);
  int dy = static_cast<int>(fy);

  s_dxRemainder = fx - dx;
  s_dyRemainder = fy - dy;

  if (dx == 0 && dy == 0)
    return;

  INPUT inp = {};
  inp.type = INPUT_MOUSE;
  inp.mi.dx = static_cast<LONG>(dx);
  inp.mi.dy = static_cast<LONG>(dy);
  inp.mi.dwFlags = MOUSEEVENTF_MOVE;
  SendInput(1, &inp, sizeof(INPUT));
}

// ── Sigmoid smooth step
// ───────────────────────────────────────────────────────
static float CalcSmoothStep(float deltaDeg, float smooth) {
  float t = deltaDeg / (smooth * 2.0f + 0.001f);
  t = fminf(t, 1.0f);
  return t * t * (3.0f - 2.0f * t); // smooth cubic: 3t²-2t³
}

// ── Update
// ────────────────────────────────────────────────────────────────────
void Aimbot::Update() {
  if (!Config::Settings.aimbot.enabled)
    return;
  if (!GetAsyncKeyState(Config::Settings.aimbot.hotkey)) {
    s_lastTarget = 0;
    return;
  }

  uintptr_t clientBase = Core::GameManager::GetClientBase();
  if (!clientBase)
    return;

  if (Config::Settings.aimbot.onlyScoped) {
    bool scoped = Core::GameManager::IsLocalScoped();
    if (!scoped)
      return;
  }

  SDK::Vector2 eyeAngles = Core::GameManager::GetLocalAngles();

  SDK::Vector3 localPos = Core::GameManager::GetLocalPos();
  localPos.z += 64.0f; // approximate eye height

  const auto players = Core::GameManager::GetRenderPlayers();

  // ── Target selection (optionally retry without lock) ──────────────────────
  bool retryWithoutLock = false;
  do {
    retryWithoutLock = false;

    const SDK::Entity *bestTarget = nullptr;
    float bestAngleDelta = Config::Settings.aimbot.fov;

    for (const auto &p : players) {
      if (!p.IsValid() || !p.IsAlive())
        continue;
      if (Config::Settings.aimbot.teamCheck && p.isTeammate)
        continue;
      if (Config::Settings.aimbot.visibleOnly && !p.isSpotted)
        continue;

      // Target lock filtering
      if (Config::Settings.aimbot.targetLock && s_lastTarget != 0) {
        if (p.address != s_lastTarget)
          continue;
      }

      if (p.bonePositions.empty() ||
          Config::Settings.aimbot.targetBone >= (int)p.bonePositions.size())
        continue;

      SDK::Vector3 bonePos =
          p.bonePositions[Config::Settings.aimbot.targetBone];
      SDK::Vector2 targetAngles = Core::Math::CalcAngle(localPos, bonePos);

      float dp = Core::Math::DeltaAngle(eyeAngles.x, targetAngles.x);
      float dy = Core::Math::DeltaAngle(eyeAngles.y, targetAngles.y);
      float delta = sqrtf(dp * dp + dy * dy);

      if (delta < bestAngleDelta) {
        bestAngleDelta = delta;
        bestTarget = &p;
      }
    }

    // If locked target is gone — release lock and scan again (no recursion)
    if (!bestTarget && Config::Settings.aimbot.targetLock &&
        s_lastTarget != 0) {
      s_lastTarget = 0;
      retryWithoutLock = true;
      continue;
    }

    if (!bestTarget) {
      s_lastTarget = 0;
      return;
    }

    // ── Compute angle to target ───────────────────────────────────────────
    SDK::Vector3 bonePos =
        bestTarget->bonePositions[Config::Settings.aimbot.targetBone];

    // Small random offset — humans rarely aim dead-center
    float jBase = s_jitterTable[s_jitterIdx & 15] *
                  Config::Settings.aimbot.jitter * 40.0f;
    s_jitterIdx = (s_jitterIdx + 1) & 15;
    bonePos.z += jBase;

    SDK::Vector2 targetAngles = Core::Math::CalcAngle(localPos, bonePos);

    float dp = Core::Math::DeltaAngle(eyeAngles.x, targetAngles.x);
    float dyw = Core::Math::DeltaAngle(eyeAngles.y, targetAngles.y);
    float totalDelta = sqrtf(dp * dp + dyw * dyw);

    // ── Dynamic human-like smooth ─────────────────────────────────────────
    float baseSmooth = Config::Settings.aimbot.smooth;
    float dynamicFactor =
        (totalDelta > 4.0f) ? (baseSmooth * 0.7f) : (baseSmooth * 1.4f);
    if (dynamicFactor < 1.0f)
      dynamicFactor = 1.0f;

    float step = CalcSmoothStep(totalDelta, dynamicFactor);
    step = fmaxf(step, 0.01f);

    // Per-axis jitter noise
    float jP =
        s_jitterTable[(s_jitterIdx + 3) & 15] * Config::Settings.aimbot.jitter;
    float jY =
        s_jitterTable[(s_jitterIdx + 7) & 15] * Config::Settings.aimbot.jitter;
    s_jitterIdx = (s_jitterIdx + 2) & 15;

    float movePitch = dp * step + jP;
    float moveYaw = dyw * step + jY;

    SendMouse(movePitch, moveYaw);

    s_lastTarget = bestTarget->address;
  } while (retryWithoutLock);
}

// ── Render: FOV circle
// ────────────────────────────────────────────────────────
void Aimbot::Render(Render::DrawList &drawList) {
  if (!Config::Settings.aimbot.enabled || !Config::Settings.aimbot.showFov)
    return;

  ImGuiIO &io = ImGui::GetIO();
  float cx = io.DisplaySize.x * 0.5f;
  float cy = io.DisplaySize.y * 0.5f;

  constexpr float CS2_HFOV = 106.0f;
  float radiusPx = (Config::Settings.aimbot.fov / CS2_HFOV) * io.DisplaySize.x;

  static float circleCol[4] = {1.0f, 1.0f, 1.0f, 0.35f};
  drawList.DrawCircle(cx, cy, radiusPx, circleCol, 64, 1.0f);
}

} // namespace Features
