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
SDK::Vector2 GameManager::renderLocalAngles = {};
float GameManager::renderLocalYaw = 0.0f;
int GameManager::renderLocalTeam = 0;
bool GameManager::renderLocalScoped = false;
uint32_t GameManager::renderLocalCrosshairHandle = 0;
SDK::BombInfo GameManager::renderBombInfo = {};

// Поля читаются отдельно через SDK::Offsets — PawnFields была удалена,
// так как её capture-паддинги не совпадали с реальными смещениями из offsets.h.

SDK::Matrix4x4 GameManager::viewMatrix = {};
uintptr_t GameManager::clientBase = 0;
std::vector<SDK::Entity> GameManager::players;

SDK::Vector3 GameManager::localPos = {};
SDK::Vector2 GameManager::localAngles = {};
float GameManager::localYaw = 0.0f;
int GameManager::localTeam = 0;
bool GameManager::localScoped = false;
uint32_t GameManager::localCrosshairHandle = 0;
SDK::BombInfo GameManager::bombInfo = {};

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

  // Убираем s_nameCache.clear() каждый кадр для оптимизации.
  // Имена переиспользуются, строка обновится поверх, если pawnHandle совпадёт.

  uintptr_t entityList =
      MemoryManager::Read<uintptr_t>(clientBase + SDK::Offsets::dwEntityList);
  if (!entityList)
    return;

  uintptr_t localPawn = MemoryManager::Read<uintptr_t>(
      clientBase + SDK::Offsets::dwLocalPlayerPawn);

  localPos = {};
  localAngles = {};
  localYaw = 0.0f;
  localTeam = 0;
  localScoped = false;
  localCrosshairHandle = 0;

  if (localPawn > 0x10000) {
    localPos = MemoryManager::Read<SDK::Vector3>(localPawn +
                                                 SDK::Offsets::m_vOldOrigin);
    localTeam = MemoryManager::Read<int>(localPawn + SDK::Offsets::m_iTeamNum);
    localAngles = MemoryManager::Read<SDK::Vector2>(
        localPawn + SDK::Offsets::m_angEyeAngles);
    localYaw = localAngles.y;
    localScoped =
        MemoryManager::Read<bool>(localPawn + SDK::Offsets::m_bIsScoped);
    localCrosshairHandle = MemoryManager::Read<uint32_t>(
        localPawn + SDK::Offsets::m_iCrosshairEntityHandle);
  }

  // ── Read Bomb State ──
  bombInfo.isPlanted = false;
  bombInfo.site = -1;
  bombInfo.timeLeft = 0.0f;

  if (SDK::Offsets::dwPlantedC4 != 0) {
    uintptr_t c4Ptr =
        MemoryManager::Read<uintptr_t>(clientBase + SDK::Offsets::dwPlantedC4);
    if (c4Ptr > 0x10000) {
      uintptr_t c4 = MemoryManager::Read<uintptr_t>(c4Ptr);
      if (!c4 || c4 < 0x10000)
        c4 = c4Ptr;

      bool ticking =
          MemoryManager::Read<bool>(c4 + SDK::Offsets::m_bBombTicking);
      if (ticking) {
        bombInfo.isPlanted = true;
        bombInfo.site =
            MemoryManager::Read<int>(c4 + SDK::Offsets::m_nBombSite);
        bombInfo.timeLeft =
            MemoryManager::Read<float>(c4 + SDK::Offsets::m_flTimerLength);
      }
    }
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
  memset(s_pawnListCache, 0, sizeof(s_pawnListCache));

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
    p.pawnHandle = pawnHandle;
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
    renderLocalAngles = localAngles;
    renderLocalYaw = localYaw;
    renderLocalTeam = localTeam;
    renderLocalScoped = localScoped;
    renderLocalCrosshairHandle = localCrosshairHandle;
    renderBombInfo = bombInfo;
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

SDK::Vector2 GameManager::GetLocalAngles() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return renderLocalAngles;
}

bool GameManager::IsLocalScoped() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return renderLocalScoped;
}

uint32_t GameManager::GetLocalCrosshairEntityHandle() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return renderLocalCrosshairHandle;
}

SDK::BombInfo GameManager::GetBombInfo() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return renderBombInfo;
}

} // namespace Core
