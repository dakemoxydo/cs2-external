#include "aimbot.h"
#include "aimbot_config.h"
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

AimbotConfig aimbotConfig;

// ─── Human-like aimbot constants ───────────────────────────────────────────
// These mirror real hand micro-variations: a human shooter doesn't move their
// mouse in a perfectly straight line nor at a constant speed.
static constexpr float DPI_FACTOR = 1.0f; // adjust if user DPI is known

// Jitter table: 16 precomputed noise offsets (degrees) applied cyclically
// so the pattern is never the same two frames in a row
static const float s_jitterTable[16] = {
    0.012f,  -0.018f, 0.007f, -0.031f, 0.022f, 0.011f,  -0.009f, 0.025f,
    -0.014f, 0.003f,  0.028f, -0.006f, 0.019f, -0.023f, 0.008f,  -0.017f,
};
static int s_jitterIdx = 0;

// ── Internal state ──────────────────────────────────────────────────────────
static uintptr_t s_lastTarget = 0; // track target continuity

// ── SendMouse: convert angle delta (degrees) to relative mouse movement ─────
// CS2 uses a fixed sensitivity factor: approx 0.022 deg per count @ sens 1.0
// We use a reasonable middle-ground. DX raw input is required.
static void SendMouse(float dpitch, float dyaw) {
  // 0.022 * sensitivityFactor → counts per degree (CS2 default sens ~2.0)
  constexpr float COUNTS_PER_DEG = 1.0f / 0.022f / 2.0f;
  int dx = static_cast<int>(dyaw * COUNTS_PER_DEG * DPI_FACTOR);
  int dy = static_cast<int>(dpitch * COUNTS_PER_DEG * DPI_FACTOR);
  if (dx != 0 || dy != 0)
    mouse_event(MOUSEEVENTF_MOVE, static_cast<DWORD>(dx),
                static_cast<DWORD>(dy), 0, 0);
}

// ── Sigmoid velocity: high value when far away, ramps down near target ──────
// Returns fractional step size (0..1) based on angular delta
static float CalcSmoothStep(float deltaDeg, float smooth) {
  // Use sigmoid-shaped ramp: fast initial movement, decelerate near target
  // distance [0..FOV] → normalized t [0..1] → sigmoid
  float t = deltaDeg / (smooth * 2.0f + 0.001f);
  t = fminf(t, 1.0f);
  // Smooth cubic: 3t²-2t³ (smoother than linear, no hard edges)
  return t * t * (3.0f - 2.0f * t);
}

// ─── Update ─────────────────────────────────────────────────────────────────
void Aimbot::Update() {
  if (!aimbotConfig.enabled)
    return;
  if (!GetAsyncKeyState(aimbotConfig.hotkey)) {
    s_lastTarget = 0;
    return;
  }

  uintptr_t localPawn = Core::MemoryManager::Read<uintptr_t>(
      Core::GameManager::GetClientBase() + SDK::Offsets::dwLocalPlayerPawn);
  if (!localPawn)
    return;

  // Skip if only-scoped mode and player is not scoped
  if (aimbotConfig.onlyScoped) {
    bool scoped =
        Core::MemoryManager::Read<bool>(localPawn + SDK::Offsets::m_bIsScoped);
    if (!scoped)
      return;
  }

  // Current eye angles (pitch, yaw)
  SDK::Vector2 eyeAngles = Core::MemoryManager::Read<SDK::Vector2>(
      localPawn + SDK::Offsets::m_angEyeAngles);

  SDK::Vector3 localPos = Core::MemoryManager::Read<SDK::Vector3>(
      localPawn + SDK::Offsets::m_vOldOrigin);
  localPos.z += 64.0f; // approximate eye height

  const auto &players = Core::GameManager::GetPlayers();

  // ── Target selection: find closest to crosshair within FOV ────────────────
  const SDK::Entity *bestTarget = nullptr;
  float bestAngleDelta = aimbotConfig.fov;

  for (const auto &p : players) {
    if (!p.IsValid() || !p.IsAlive())
      continue;
    if (aimbotConfig.teamCheck && p.isTeammate)
      continue;
    if (p.bonePositions.empty() ||
        aimbotConfig.targetBone >= (int)p.bonePositions.size())
      continue;

    SDK::Vector3 bonePos = p.bonePositions[aimbotConfig.targetBone];
    SDK::Vector2 targetAngles = Core::Math::CalcAngle(localPos, bonePos);

    float dp = Core::Math::DeltaAngle(eyeAngles.x, targetAngles.x);
    float dy = Core::Math::DeltaAngle(eyeAngles.y, targetAngles.y);
    float delta = sqrtf(dp * dp + dy * dy);

    if (delta < bestAngleDelta) {
      bestAngleDelta = delta;
      bestTarget = &p;
    }
  }

  if (!bestTarget) {
    s_lastTarget = 0;
    return;
  }

  // ── Compute angle to target ───────────────────────────────────────────────
  SDK::Vector3 bonePos = bestTarget->bonePositions[aimbotConfig.targetBone];

  // Small random offset from bone center — humans rarely aim dead-center
  float jBase = s_jitterTable[s_jitterIdx & 15] * aimbotConfig.jitter * 40.0f;
  s_jitterIdx = (s_jitterIdx + 1) & 15;
  bonePos.z += jBase;

  SDK::Vector2 targetAngles = Core::Math::CalcAngle(localPos, bonePos);

  float dp = Core::Math::DeltaAngle(eyeAngles.x, targetAngles.x);
  float dy = Core::Math::DeltaAngle(eyeAngles.y, targetAngles.y);
  float totalDelta = sqrtf(dp * dp + dy * dy);

  // ── Human-like sigmoid smooth ─────────────────────────────────────────────
  // Step fraction: fast when far (>smooth), slow when close (<0.5°)
  float step = CalcSmoothStep(totalDelta, aimbotConfig.smooth);
  step = fmaxf(step, 0.01f); // always move a tiny bit

  // Apply per-axis jitter noise (independent on pitch/yaw for realism)
  float jP = s_jitterTable[(s_jitterIdx + 3) & 15] * aimbotConfig.jitter;
  float jY = s_jitterTable[(s_jitterIdx + 7) & 15] * aimbotConfig.jitter;
  s_jitterIdx = (s_jitterIdx + 2) & 15;

  float movePitch = dp * step + jP;
  float moveYaw = dy * step + jY;

  SendMouse(movePitch, moveYaw);

  s_lastTarget = bestTarget->address;
}

void Aimbot::Render(Render::DrawList &drawList) {
  if (!aimbotConfig.enabled || !aimbotConfig.showFov)
    return;

  // FOV circle — draw at screen center.
  // We approximate FOV degrees → screen pixels using DisplaySize.
  // At 90° hFOV a 1° offset ≈ screenW/90 px. CS2 default is ~106° hFOV.
  ImGuiIO &io = ImGui::GetIO();
  float cx = io.DisplaySize.x * 0.5f;
  float cy = io.DisplaySize.y * 0.5f;

  // Convert aimbot FOV (degrees) to screen-space radius (pixels)
  constexpr float CS2_HFOV = 106.0f; // horizontal FOV in degrees
  float radiusPx = (aimbotConfig.fov / CS2_HFOV) * io.DisplaySize.x;

  // Draw a thin circle; color = white with low alpha
  static float circleCol[4] = {1.0f, 1.0f, 1.0f, 0.35f};
  drawList.DrawCircle(cx, cy, radiusPx, circleCol, 64, 1.0f);
}

} // namespace Features
