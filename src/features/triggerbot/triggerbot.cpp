#include "triggerbot.h"
#include "core/game/game_manager.h"
#include "core/memory/memory_manager.h"
#include "core/sdk/offsets.h"
#include "render/draw/draw_list.h"
#include "triggerbot_config.h"
#include <chrono>
#include <cstdlib>
#include <windows.h>


namespace Features {

TriggerbotConfig triggerbotConfig;

// ─── State machine ──────────────────────────────────────────────────────────
enum class TBState { IDLE, TARGET_FOUND, WAITING, SHOOTING, COOLDOWN };
static TBState s_state = TBState::IDLE;
static std::chrono::steady_clock::time_point s_timer;
static int s_delayMs = 0;

// ─── Helpers ─────────────────────────────────────────────────────────────────
static int RandRange(int lo, int hi) {
  if (lo >= hi)
    return lo;
  return lo + rand() % (hi - lo + 1);
}

static void ClickDown() { mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0); }
static void ClickUp() { mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0); }

static auto NowMs() { return std::chrono::steady_clock::now(); }
static long long ElapsedMs(std::chrono::steady_clock::time_point t) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(NowMs() - t)
      .count();
}

// ─── Update ──────────────────────────────────────────────────────────────────
void Triggerbot::Update() {
  if (!triggerbotConfig.enabled) {
    s_state = TBState::IDLE;
    return;
  }

  bool keyHeld = (GetAsyncKeyState(triggerbotConfig.hotkey) & 0x8000) != 0;
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

  // Check if the handle points to a valid enemy in our player list
  bool onEnemy = false;
  if (crossHairHandle != 0 && crossHairHandle != 0xFFFFFFFF) {
    for (const auto &p : Core::GameManager::GetPlayers()) {
      if (!p.IsValid() || !p.IsAlive())
        continue;
      if (triggerbotConfig.teamCheck && p.isTeammate)
        continue;

      // The lower 14 bits of the entity handle encode the serial/index
      // We match by address — if it exists in our valid player list it's an
      // enemy
      uint32_t localHandle = Core::MemoryManager::Read<uint32_t>(
          p.address + 0x10); // entity index field (common offset)
      if (localHandle == crossHairHandle) {
        onEnemy = true;
        break;
      }
    }
    // Simpler fallback: if crosshair handle != 0 and entity type is player
    // Just check that the entity exists in our enemy list by checking address
    if (!onEnemy) {
      // Build entity lookup from handle if normal approach fails
      uintptr_t entityList = Core::MemoryManager::Read<uintptr_t>(
          Core::GameManager::GetClientBase() + SDK::Offsets::dwEntityList);
      if (entityList) {
        uintptr_t pawnListEntry = Core::MemoryManager::Read<uintptr_t>(
            entityList + 0x10 + 0x8 * ((crossHairHandle & 0x7FFF) >> 9));
        if (pawnListEntry) {
          uintptr_t entPawn = Core::MemoryManager::Read<uintptr_t>(
              pawnListEntry + 0x70 * (crossHairHandle & 0x1FF));
          if (entPawn > 0x10000) {
            int health = Core::MemoryManager::Read<int>(
                entPawn + SDK::Offsets::m_iHealth);
            int team = Core::MemoryManager::Read<uint8_t>(
                entPawn + SDK::Offsets::m_iTeamNum);
            int localTeam = Core::MemoryManager::Read<uint8_t>(
                localPawn + SDK::Offsets::m_iTeamNum);
            if (health > 0 && health <= 100 &&
                !(triggerbotConfig.teamCheck && team == localTeam))
              onEnemy = true;
          }
        }
      }
    }
  }

  // ── State machine ────────────────────────────────────────────────────────
  switch (s_state) {
  case TBState::IDLE:
    if (onEnemy) {
      s_delayMs =
          RandRange(triggerbotConfig.delayMin, triggerbotConfig.delayMax);
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
    // Cooldown: 80–200ms before next trigger (humanized)
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
