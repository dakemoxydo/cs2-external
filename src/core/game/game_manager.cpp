#include "game_manager.h"
#include "../memory/memory_manager.h"
#include "../process/module.h"
#include "../sdk/offsets.h"
#include <cmath>
#include <iostream>

namespace Core {

// Member offsets verified against reference ESP (git
// cs2/src/core/offsets/Offsets.hpp)
namespace MO { // short alias for this file
static constexpr ptrdiff_t m_hPawn = 0x6C4;
static constexpr ptrdiff_t m_iszPlayerName = 0x6F8;
static constexpr ptrdiff_t m_iHealth = 0x354;
static constexpr ptrdiff_t m_iTeamNum = 0x3F3;
static constexpr ptrdiff_t m_vOldOrigin = 0x1588;
static constexpr ptrdiff_t m_pGameSceneNode = 0x338;
static constexpr ptrdiff_t m_pClippingWeapon = 0x3DC0;
} // namespace MO

// ── Pawn fields to batch-read in one RPM call ──
// We read a small "pawn summary" struct instead of 4 individual calls
struct PawnSummary {
  // offset 0x338 starts too far in — we read from 0x338 as base
  // key fields:
  uintptr_t m_pGameSceneNode; // +0x000 (relative to 0x338)
};

// PawnBlock: batch-read from pawn+0x350 covering health/team/origin
struct PawnFields {
  uint8_t _pad0[4];          // 0x350
  int m_iHealth;             // 0x354
  uint8_t _pad1[0x9E];       // 0x358-0x3F2
  uint8_t m_iTeamNum;        // 0x3F3
  uint8_t _pad2[0x194];      // 0x3F4-0x1587
  SDK::Vector3 m_vOldOrigin; // 0x1588
};

SDK::Matrix4x4 GameManager::viewMatrix = {};
uintptr_t GameManager::clientBase = 0;
std::vector<SDK::Entity> GameManager::players;

// Cache: pawnListEntry per chunk index (chunk = (handle & 0x7FFF) >> 9)
// Invalidated each frame since base addresses may change
static constexpr int LIST_CHUNKS = 64;
static uintptr_t s_pawnListCache[LIST_CHUNKS] = {};

bool GameManager::Init() {
  clientBase = Module::GetBaseAddress(L"client.dll");
  if (!clientBase)
    return false;
  std::cout << "[DEBUG] client.dll base: 0x" << std::hex << clientBase
            << std::dec << std::endl;
  return true;
}

void GameManager::Update() {
  if (!clientBase) {
    if (!Init())
      return;
  }

  // ── View matrix (single read) ──
  viewMatrix = MemoryManager::Read<SDK::Matrix4x4>(clientBase +
                                                   SDK::Offsets::dwViewMatrix);

  players.clear();
  // Reuse vector memory from last frame
  players.reserve(16);

  uintptr_t entityList =
      MemoryManager::Read<uintptr_t>(clientBase + SDK::Offsets::dwEntityList);
  if (!entityList)
    return;

  uintptr_t localPawn = MemoryManager::Read<uintptr_t>(
      clientBase + SDK::Offsets::dwLocalPlayerPawn);

  SDK::Vector3 localPos = {};
  int localTeam = 0;
  if (localPawn > 0x10000) {
    localPos = MemoryManager::Read<SDK::Vector3>(localPawn + MO::m_vOldOrigin);
    localTeam = MemoryManager::Read<int>(localPawn + MO::m_iTeamNum);
  }

  // ── Batch-read first chunk of controller pointers (64 slots × 0x70 each) ──
  // All 64 controllers live in one contiguous block starting at listEntry+0x70
  uintptr_t listEntry = MemoryManager::Read<uintptr_t>(entityList + 0x10);
  if (!listEntry)
    return;

  constexpr int MAX_PLAYERS = 64;
  uintptr_t controllers[MAX_PLAYERS] = {};
  // Each slot is at listEntry + (i+1)*0x70, which is NOT packed.
  // Read individual pointers from the entity list.

  // ── Invalidate pawn list chunk cache each frame ──
  for (int c = 0; c < LIST_CHUNKS; c++)
    s_pawnListCache[c] = 0;

  for (int i = 0; i < MAX_PLAYERS; i++) {
    uintptr_t controller =
        MemoryManager::Read<uintptr_t>(listEntry + (i + 1) * 0x70);
    if (!controller || controller < 0x10000)
      continue;

    uint32_t pawnHandle =
        MemoryManager::Read<uint32_t>(controller + MO::m_hPawn);
    if (!pawnHandle || pawnHandle == 0xFFFFFFFF)
      continue;

    // Cached pawn list chunk lookup
    int chunkIdx = (int)((pawnHandle & 0x7FFF) >> 9);
    if (chunkIdx < 0 || chunkIdx >= LIST_CHUNKS)
      continue;

    if (!s_pawnListCache[chunkIdx]) {
      s_pawnListCache[chunkIdx] =
          MemoryManager::Read<uintptr_t>(entityList + 0x10 + 0x8 * chunkIdx);
    }
    uintptr_t pawnListEntry = s_pawnListCache[chunkIdx];
    if (!pawnListEntry)
      continue;

    uintptr_t pawn = MemoryManager::Read<uintptr_t>(
        pawnListEntry + 0x70 * (pawnHandle & 0x1FF));
    if (!pawn || pawn < 0x10000 || pawn == localPawn)
      continue;

    // ── Batch-read pawn key fields (one RPM for health+team+origin) ──
    // Fields span from 0x350 to 0x1594 (too wide for single call ~0x1244 bytes)
    // So we do two read: one small struct for health/team, one for origin
    // Actually we split at 0x3F3 and 0x1588 — too far. Optimal: read
    // health+team together (they are close: 0x354 and 0x3F3 = delta 0x9F), then
    // origin separately. Better: just 3 reads total vs 4 before (health, team
    // combined as uint16 would corrupt)

    // Read health
    int health = MemoryManager::Read<int>(pawn + MO::m_iHealth);
    if (health <= 0 || health > 100)
      continue;

    // Read team
    int team = MemoryManager::Read<uint8_t>(pawn + (ptrdiff_t)MO::m_iTeamNum);
    if (team != 2 && team != 3)
      continue;

    // Read origin
    SDK::Vector3 position =
        MemoryManager::Read<SDK::Vector3>(pawn + MO::m_vOldOrigin);
    if (position.x == 0.0f && position.y == 0.0f && position.z == 0.0f)
      continue;

    // ── Build entity ──
    SDK::Entity p;
    p.address = pawn;
    p.controllerAddress = controller;
    p.health = health;
    p.team = team;
    p.isTeammate = (localTeam != 0 && team == localTeam);
    p.position = position;

    // Name (single ReadRaw)
    char nameBuffer[128] = {};
    MemoryManager::ReadRaw(controller + MO::m_iszPlayerName, nameBuffer,
                           sizeof(nameBuffer) - 1);
    p.name = nameBuffer;

    // Distance
    {
      float dx = position.x - localPos.x;
      float dy = position.y - localPos.y;
      float dz = position.z - localPos.z;
      p.distance = sqrtf(dx * dx + dy * dy + dz * dz) / 100.0f;
    }

    // Weapon (triple-deref pointer chain)
    {
      uintptr_t cw =
          MemoryManager::Read<uintptr_t>(pawn + MO::m_pClippingWeapon);
      if (cw > 0x10000) {
        uintptr_t wPtr = MemoryManager::Read<uintptr_t>(cw + 0x10);
        if (wPtr > 0x10000) {
          uintptr_t nPtr = MemoryManager::Read<uintptr_t>(wPtr + 0x20);
          if (nPtr > 0x10000) {
            char wb[64] = {};
            MemoryManager::ReadRaw(nPtr, wb, sizeof(wb) - 1);
            p.weapon = wb;
            if (p.weapon.rfind("weapon_", 0) == 0)
              p.weapon = p.weapon.substr(7);
          }
        }
      }
    }

    // ── Skeleton bones (one batch ReadRaw) ──
    uintptr_t gameScene =
        MemoryManager::Read<uintptr_t>(pawn + MO::m_pGameSceneNode);
    if (gameScene > 0x10000) {
      uintptr_t boneArray =
          MemoryManager::Read<uintptr_t>(gameScene + (0x160 + 0x80));
      if (boneArray > 0x10000) {
        BoneData rawBones[BONE_COUNT];
        if (MemoryManager::ReadRaw(boneArray, rawBones, sizeof(rawBones))) {
          p.bonePositions.resize(BONE_COUNT);
          for (int b = 0; b < BONE_COUNT; b++)
            p.bonePositions[b] = rawBones[b].pos;
        }
      }
    }

    players.emplace_back(std::move(p));
  }
}

SDK::Matrix4x4 GameManager::GetViewMatrix() { return viewMatrix; }
uintptr_t GameManager::GetClientBase() { return clientBase; }
const std::vector<SDK::Entity> &GameManager::GetPlayers() { return players; }

} // namespace Core
