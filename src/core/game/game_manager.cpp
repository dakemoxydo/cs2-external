#include "game_manager.h"
#include "../memory/memory_manager.h"
#include "../process/module.h"
#include "../sdk/offsets.h"
#include <iostream>

namespace Core {

// Member offsets verified against reference ESP (git
// cs2/src/core/offsets/Offsets.hpp)
namespace MemberOffsets {
static constexpr ptrdiff_t m_hPawn = 0x6C4;
static constexpr ptrdiff_t m_iszPlayerName = 0x6F8;
static constexpr ptrdiff_t m_iHealth = 0x354;
static constexpr ptrdiff_t m_iTeamNum = 0x3F3;
static constexpr ptrdiff_t m_vOldOrigin = 0x1588;
static constexpr ptrdiff_t m_pGameSceneNode = 0x338;
} // namespace MemberOffsets

SDK::Matrix4x4 GameManager::viewMatrix = {};
uintptr_t GameManager::clientBase = 0;
std::vector<SDK::Entity> GameManager::players;

bool GameManager::Init() {
  clientBase = Module::GetBaseAddress(L"client.dll");
  if (!clientBase) {
    return false;
  }
  std::cout << "[DEBUG] client.dll base: 0x" << std::hex << clientBase
            << std::dec << std::endl;
  return true;
}

void GameManager::Update() {
  if (!clientBase) {
    if (!Init()) {
      return;
    }
  }

  viewMatrix = MemoryManager::Read<SDK::Matrix4x4>(clientBase +
                                                   SDK::Offsets::dwViewMatrix);
  players.clear();

  uintptr_t entityList =
      MemoryManager::Read<uintptr_t>(clientBase + SDK::Offsets::dwEntityList);
  if (!entityList)
    return;

  // First chunk pointer — the array of controller pointers lives here
  uintptr_t listEntry = MemoryManager::Read<uintptr_t>(entityList + 0x10);
  if (!listEntry)
    return;

  uintptr_t localPawn = MemoryManager::Read<uintptr_t>(
      clientBase + SDK::Offsets::dwLocalPlayerPawn);

  // Read local player's team so we can tag teammates
  int localTeam = 0;
  if (localPawn > 0x10000)
    localTeam = MemoryManager::Read<int>(localPawn + MemberOffsets::m_iTeamNum);

  for (int i = 0; i < 64; i++) {
    // Reference pattern (Player.cpp line 37): controller = read(le + (index+1)
    // * 0x70)
    uintptr_t controller =
        MemoryManager::Read<uintptr_t>(listEntry + (i + 1) * 0x70);
    if (!controller || controller < 0x10000)
      continue;

    uintptr_t pawnHandle =
        MemoryManager::Read<uintptr_t>(controller + MemberOffsets::m_hPawn);
    if (!pawnHandle || pawnHandle == 0xFFFFFFFF)
      continue;

    // Reference pawn lookup (Player.cpp lines 59, 64):
    // list = read(entityList + 0x10 + 0x8 * ((handle & 0x7FFF) >> 9))
    // pawn = read(list + 0x70 * (handle & 0x1FF))
    uintptr_t pawnListEntry = MemoryManager::Read<uintptr_t>(
        entityList + 0x10 + 0x8 * ((pawnHandle & 0x7FFF) >> 9));
    if (!pawnListEntry)
      continue;

    uintptr_t pawn = MemoryManager::Read<uintptr_t>(
        pawnListEntry + 0x70 * (pawnHandle & 0x1FF));
    if (!pawn || pawn < 0x10000)
      continue;

    // Skip local player's own pawn
    if (pawn == localPawn)
      continue;

    int health = MemoryManager::Read<int>(pawn + MemberOffsets::m_iHealth);
    if (health <= 0 || health > 100)
      continue;

    int team = MemoryManager::Read<int>(pawn + MemberOffsets::m_iTeamNum);
    if (team != 2 && team != 3)
      continue;

    SDK::Vector3 position =
        MemoryManager::Read<SDK::Vector3>(pawn + MemberOffsets::m_vOldOrigin);
    if (position.x == 0 && position.y == 0 && position.z == 0)
      continue;

    SDK::Entity p;
    p.address = pawn;
    p.controllerAddress = controller;
    p.health = health;
    p.team = team;
    p.isTeammate = (localTeam != 0 && team == localTeam);
    p.position = position;

    char nameBuffer[128] = {};
    MemoryManager::ReadRaw(controller + MemberOffsets::m_iszPlayerName,
                           nameBuffer, sizeof(nameBuffer) - 1);
    p.name = std::string(nameBuffer);

    players.emplace_back(p);
  }
}

SDK::Matrix4x4 GameManager::GetViewMatrix() { return viewMatrix; }

uintptr_t GameManager::GetClientBase() { return clientBase; }

const std::vector<SDK::Entity> &GameManager::GetPlayers() { return players; }

} // namespace Core
