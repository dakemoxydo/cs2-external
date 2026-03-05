#include "triggerbot.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "core/memory/memory_manager.h"
#include "core/sdk/offsets.h"
#include "render/draw/draw_list.h"
#include "triggerbot_config.h"
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <windows.h>

namespace Features {

// ── srand once
// ────────────────────────────────────────────────────────────────
static void EnsureRandSeeded() {
  static bool seeded = false;
  if (!seeded) {
    srand(static_cast<unsigned int>(time(nullptr)));
    seeded = true;
  }
}

// ── State machine
// ─────────────────────────────────────────────────────────────
enum class TBState { IDLE, TARGET_FOUND, WAITING, SHOOTING, COOLDOWN };
static TBState s_state = TBState::IDLE;
static std::chrono::steady_clock::time_point s_timer;
static int s_delayMs = 0;

// ── Helpers
// ───────────────────────────────────────────────────────────────────
static int RandRange(int lo, int hi) {
  if (lo >= hi)
    return lo;
  return lo + rand() % (hi - lo + 1);
}

static void ClickDown() {
  INPUT inp = {};
  inp.type = INPUT_MOUSE;
  inp.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
  SendInput(1, &inp, sizeof(INPUT));
}
static void ClickUp() {
  INPUT inp = {};
  inp.type = INPUT_MOUSE;
  inp.mi.dwFlags = MOUSEEVENTF_LEFTUP;
  SendInput(1, &inp, sizeof(INPUT));
}

static auto NowMs() { return std::chrono::steady_clock::now(); }
static long long ElapsedMs(std::chrono::steady_clock::time_point t) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(NowMs() - t)
      .count();
}

// ── Update
// ────────────────────────────────────────────────────────────────────
void Triggerbot::Update() {
  EnsureRandSeeded();

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

  uintptr_t localPawn = Core::MemoryManager::Read<uintptr_t>(
      Core::GameManager::GetClientBase() + SDK::Offsets::dwLocalPlayerPawn);
  if (!localPawn)
    return;

  // Read crosshair entity handle
  uint32_t crossHairHandle = Core::MemoryManager::Read<uint32_t>(
      localPawn + SDK::Offsets::m_iCrosshairEntityHandle);

  // Check if the handle points to a valid enemy in our player list.
  // We rely entirely on GameManager's already-built player list — no duplicate
  // RPM.
  bool onEnemy = false;
  if (crossHairHandle != 0 && crossHairHandle != 0xFFFFFFFF) {
    for (const auto &p : Core::GameManager::GetRenderPlayers()) {
      if (!p.IsValid() || !p.IsAlive())
        continue;
      if (Config::Settings.triggerbot.teamCheck && p.isTeammate)
        continue;

      // Match: read entity handle field from pawn and compare
      uint32_t entHandle =
          Core::MemoryManager::Read<uint32_t>(p.address + 0x10);
      if (entHandle == crossHairHandle) {
        onEnemy = true;
        break;
      }
    }
  }

  // ── State machine ─────────────────────────────────────────────────────────
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
      ClickDown();
      s_timer = NowMs();
      s_state = TBState::SHOOTING;
    }
    break;

  case TBState::SHOOTING:
    if (ElapsedMs(s_timer) >= 25) { // hold click for ~25ms
      ClickUp();
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
  // No visual rendering
}

} // namespace Features
