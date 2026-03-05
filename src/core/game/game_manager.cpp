#include "game_manager.h"
#include "../memory/memory_manager.h"
#include "../process/module.h"
#include "../sdk/offsets.h"
#include <cmath>
#include <iostream>
#include <unordered_map>

namespace Core {

// Thread synchronization primitives
std::shared_mutex GameManager::stateMutex;
SDK::Matrix4x4 GameManager::renderViewMatrix = {};
std::vector<SDK::Entity> GameManager::renderPlayers;
SDK::Vector3 GameManager::renderLocalPos = {};
float GameManager::renderLocalYaw = 0.0f;
int GameManager::renderLocalTeam = 0;

// Поля читаются отдельно через SDK::Offsets — PawnFields была удалена,
// так как её capture-паддинги не совпадали с реальными смещениями из offsets.h.

SDK::Matrix4x4 GameManager::viewMatrix = {};
uintptr_t GameManager::clientBase = 0;
std::vector<SDK::Entity> GameManager::players;

SDK::Vector3 GameManager::localPos = {};
float GameManager::localYaw = 0.0f;
int GameManager::localTeam = 0;

// Cache: pawnListEntry per chunk index
static constexpr int LIST_CHUNKS = 64;
static uintptr_t s_pawnListCache[LIST_CHUNKS] = {};
static std::unordered_map<uint32_t, std::string> s_nameCache;

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
  players.reserve(16);

  // Сбрасываем кэш имён каждый кадр — pawnHandle может переиспользоваться
  // после смерти/смены раунда, и тогда кэш вернёт устаревшее имя.
  s_nameCache.clear();

  uintptr_t entityList =
      MemoryManager::Read<uintptr_t>(clientBase + SDK::Offsets::dwEntityList);
  if (!entityList)
    return;

  uintptr_t localPawn = MemoryManager::Read<uintptr_t>(
      clientBase + SDK::Offsets::dwLocalPlayerPawn);

  localPos = {};
  localTeam = 0;
  if (localPawn > 0x10000) {
    localPos = MemoryManager::Read<SDK::Vector3>(localPawn +
                                                 SDK::Offsets::m_vOldOrigin);
    localTeam = MemoryManager::Read<int>(localPawn + SDK::Offsets::m_iTeamNum);
    SDK::Vector2 eyeAngles = MemoryManager::Read<SDK::Vector2>(
        localPawn + SDK::Offsets::m_angEyeAngles);
    localYaw = eyeAngles.y;
  }

  // ── Batch-read first chunk of controller pointers ──
  uintptr_t listEntry = MemoryManager::Read<uintptr_t>(entityList + 0x10);
  if (!listEntry)
    return;

  constexpr int MAX_PLAYERS = 64;
  // В CS2 каждая запись entity list chunk — CEntityIdentity (0x70 байт).
  // Указатель на entity лежит по +0x00 каждой записи.
  // Контроллеры: индексы 1..64 (0 = world/null), шаг 0x70 байт.
  uintptr_t controllerPointers[MAX_PLAYERS] = {};
  for (int i = 0; i < MAX_PLAYERS; i++) {
    controllerPointers[i] =
        MemoryManager::Read<uintptr_t>(listEntry + (i + 1) * 0x70);
  }

  // ── Invalidate pawn list chunk cache each frame ──
  for (int c = 0; c < LIST_CHUNKS; c++)
    s_pawnListCache[c] = 0;

  // Find localSlot for SpottedByMask
  int localSlot = 0;
  for (int i = 0; i < MAX_PLAYERS; i++) {
    uintptr_t controller = controllerPointers[i];
    if (!controller)
      continue;
    uint32_t pawnHandle =
        MemoryManager::Read<uint32_t>(controller + SDK::Offsets::m_hPawn);
    if (pawnHandle && pawnHandle != 0xFFFFFFFF) {
      bool isLocal = MemoryManager::Read<bool>(
          controller + SDK::Offsets::m_bIsLocalPlayerController);
      if (isLocal) {
        localSlot = (pawnHandle & 0x7FFF) - 1;
        break;
      }
    }
  }

  for (int i = 0; i < MAX_PLAYERS; i++) {
    uintptr_t controller = controllerPointers[i];
    if (!controller || controller < 0x10000)
      continue;

    uint32_t pawnHandle =
        MemoryManager::Read<uint32_t>(controller + SDK::Offsets::m_hPawn);
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

    // CEntityIdentity stride = 0x70 байт, entity ptr at +0x00 каждой записи
    uintptr_t pawn = MemoryManager::Read<uintptr_t>(
        pawnListEntry + (pawnHandle & 0x1FF) * 0x70);
    if (!pawn || pawn < 0x10000 || pawn == localPawn)
      continue;

    // Читаем health, team, origin через offsets.h — строго по правилам проекта
    int health = MemoryManager::Read<int>(pawn + SDK::Offsets::m_iHealth);
    if (health <= 0 || health > 100)
      continue;

    int team = MemoryManager::Read<int>(pawn + SDK::Offsets::m_iTeamNum);
    if (team != 2 && team != 3)
      continue;

    SDK::Vector3 position =
        MemoryManager::Read<SDK::Vector3>(pawn + SDK::Offsets::m_vOldOrigin);
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

    // Read spotted state
    uint32_t spottedMask = MemoryManager::Read<uint32_t>(
        pawn + SDK::Offsets::m_entitySpottedState +
        SDK::Offsets::m_bSpottedByMaskOffset);
    p.isSpotted = (spottedMask & (1 << localSlot)) != 0;

    // Name (Cached)
    if (s_nameCache.find(pawnHandle) == s_nameCache.end()) {
      char nameBuffer[128] = {};
      MemoryManager::ReadRaw(controller + SDK::Offsets::m_iszPlayerName,
                             nameBuffer, sizeof(nameBuffer) - 1);
      s_nameCache[pawnHandle] = nameBuffer;
    }
    p.name = s_nameCache[pawnHandle];

    // Distance
    {
      float dx = position.x - localPos.x;
      float dy = position.y - localPos.y;
      float dz = position.z - localPos.z;
      p.distance = sqrtf(dx * dx + dy * dy + dz * dz) / 100.0f;
    }

    // Weapon (triple-deref pointer chain)
    {
      uintptr_t cw = MemoryManager::Read<uintptr_t>(
          pawn + SDK::Offsets::m_pClippingWeapon);
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
        MemoryManager::Read<uintptr_t>(pawn + SDK::Offsets::m_pGameSceneNode);
    if (gameScene > 0x10000) {
      uintptr_t boneArray = MemoryManager::Read<uintptr_t>(
          gameScene + SDK::Offsets::m_boneArrayOffset);
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

  // ── Push to frontend (Thread-Safe Buffer) ──
  {
    std::unique_lock<std::shared_mutex> lock(stateMutex);
    renderPlayers = players;
    renderViewMatrix = viewMatrix;
    renderLocalPos = localPos;
    renderLocalYaw = localYaw;
    renderLocalTeam = localTeam;
  }
}

SDK::Matrix4x4 GameManager::GetViewMatrix() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return renderViewMatrix;
}

uintptr_t GameManager::GetClientBase() { return clientBase; }

std::vector<SDK::Entity> GameManager::GetRenderPlayers() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return renderPlayers;
}

SDK::Vector3 GameManager::GetLocalPos() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return renderLocalPos;
}

float GameManager::GetLocalYaw() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return renderLocalYaw;
}

int GameManager::GetLocalTeam() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return renderLocalTeam;
}

} // namespace Core
