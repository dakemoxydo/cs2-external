#include "game_manager.h"
#include "../memory/memory_manager.h"
#include "../process/module.h"
#include "../sdk/offsets.h"
#include "../sdk/entity_classes.h"
#include <cmath>
#include <unordered_map>

namespace Core {

// Thread synchronization primitives
std::shared_mutex GameManager::stateMutex;
SDK::Matrix4x4 GameManager::cachedViewMatrix = {};

std::vector<SDK::Entity> GameManager::playerBuffers[2];
std::atomic<int> GameManager::activeBufferIndex{0};

std::atomic<bool> GameManager::s_readBones{false};
std::atomic<bool> GameManager::s_readWeapons{false};
SDK::Vector3 GameManager::cachedLocalPos = {};
SDK::Vector3 GameManager::cachedLocalEyePos = {};
SDK::Vector2 GameManager::cachedLocalAngles = {};
SDK::Vector2 GameManager::cachedLocalAimPunch = {};
int GameManager::cachedLocalShotsFired = 0;
int GameManager::cachedLocalTeam = 0;
bool GameManager::cachedLocalScoped = false;
uint32_t GameManager::cachedLocalCrosshairHandle = 0;
uintptr_t GameManager::cachedLocalPawn = 0;
uintptr_t GameManager::cachedEntityList = 0;
SDK::BombInfo GameManager::cachedBombInfo = {};

// Поля читаются отдельно через SDK::Offsets — PawnFields была удалена,
// так как её capture-паддинги не совпадали с реальными смещениями из offsets.h.

SDK::Matrix4x4 GameManager::viewMatrix = {};
uintptr_t GameManager::clientBase = 0;
std::vector<SDK::Entity> GameManager::players;

SDK::Vector3 GameManager::localPos = {};
SDK::Vector3 GameManager::localEyePos = {};
SDK::Vector2 GameManager::localAngles = {};
SDK::Vector2 GameManager::localAimPunch = {};
int GameManager::localShotsFired = 0;
int GameManager::localTeam = 0;
bool GameManager::localScoped = false;
uint32_t GameManager::localCrosshairHandle = 0;
uintptr_t GameManager::localPawn = 0;
uintptr_t GameManager::entityList = 0;
SDK::BombInfo GameManager::bombInfo = {};

// Cache: pawnListEntry per chunk index
static constexpr int LIST_CHUNKS = 64;
static uintptr_t s_pawnListCache[LIST_CHUNKS] = {};
static std::unordered_map<uint32_t, std::string> s_nameCache;

bool GameManager::Init() {
  clientBase = Module::GetBaseAddress(L"client.dll");
  return clientBase != 0;
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

  entityList =
      MemoryManager::Read<uintptr_t>(clientBase + SDK::Offsets::dwEntityList);
  if (!entityList)
    return;

  SDK::CEntityList entityListObj(entityList);

  // ── Cache Invalidation ──
  // If the first chunk address changes, we are on a new map or reconnected
  static uintptr_t s_lastFirstChunk = 0;
  uintptr_t currentFirstChunk = entityListObj.GetListEntry(0);
  if (currentFirstChunk != s_lastFirstChunk) {
    s_lastFirstChunk = currentFirstChunk;
    memset(s_pawnListCache, 0, sizeof(s_pawnListCache));
    s_nameCache.clear();
  }

  SDK::CPlayerPawn localPlayer(MemoryManager::Read<uintptr_t>(clientBase + SDK::Offsets::dwLocalPlayerPawn));
  localPawn = localPlayer.GetAddress();

  localPos = {};
  localEyePos = {};
  localAngles = {};
  localAimPunch = {};
  localShotsFired = 0;
  localTeam = 0;
  localScoped = false;
  localCrosshairHandle = 0;

  if (localPlayer.IsValid()) {
    localPos = localPlayer.GetOldOrigin();
    localEyePos = localPlayer.GetCameraPos();
    localTeam = localPlayer.GetTeam();
    localAngles = localPlayer.GetEyeAngles();
    localAimPunch = localPlayer.GetAimPunch();
    localShotsFired = localPlayer.GetShotsFired();
    localScoped = localPlayer.IsScoped();
    localCrosshairHandle = localPlayer.GetCrosshairEntityHandle();
  }

  // ── Read Bomb State ──
  bombInfo.isPlanted = false;
  bombInfo.site = -1;
  bombInfo.timeLeft = 0.0f;

  if (SDK::Offsets::dwPlantedC4 != 0) {
    uintptr_t c4Ptr =
        MemoryManager::Read<uintptr_t>(clientBase + SDK::Offsets::dwPlantedC4);
    if (c4Ptr > 0x10000) {
      uintptr_t c4Addr = MemoryManager::Read<uintptr_t>(c4Ptr);
      if (!c4Addr || c4Addr < 0x10000)
        c4Addr = c4Ptr;
      
      SDK::CPlantedC4 c4(c4Addr);
      if (c4.IsTicking()) {
        bombInfo.isPlanted = true;
        bombInfo.site = c4.GetSite();
        bombInfo.timeLeft = c4.GetTimeLeft();
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
    SDK::CPlayerController controller(controllerPointers[i]);
    if (!controller.IsValid())
      continue;
    uint32_t pawnHandle = controller.GetPawnHandle();
    if (pawnHandle && pawnHandle != 0xFFFFFFFF) {
      if (controller.IsLocalPlayerController()) {
        localSlot = (pawnHandle & 0x7FFF) - 1;
        break;
      }
    }
  }

  for (int i = 0; i < MAX_PLAYERS; i++) {
    SDK::CPlayerController controller(controllerPointers[i]);
    if (!controller.IsValid())
      continue;

    uint32_t pawnHandle = controller.GetPawnHandle();
    if (!pawnHandle || pawnHandle == 0xFFFFFFFF)
      continue;

    // Cached pawn list chunk lookup
    int chunkIdx = (int)((pawnHandle & 0x7FFF) >> 9);
    if (chunkIdx < 0 || chunkIdx >= LIST_CHUNKS)
      continue;

    if (!s_pawnListCache[chunkIdx]) {
      s_pawnListCache[chunkIdx] = entityListObj.GetListEntry(chunkIdx);
    }
    
    SDK::CPlayerPawn pawn = entityListObj.GetPawnFromHandle(pawnHandle, s_pawnListCache[chunkIdx]);
    if (!pawn.IsValid() || pawn.GetAddress() == localPawn)
      continue;

    int health = pawn.GetHealth();
    if (health <= 0 || health > 100)
      continue;

    int team = pawn.GetTeam();
    if (team != 2 && team != 3)
      continue;

    SDK::Vector3 position = pawn.GetOldOrigin();
    if (position.x == 0.0f && position.y == 0.0f && position.z == 0.0f)
      continue;

    // ── Build entity ──
    SDK::Entity p;
    p.address = pawn.GetAddress();
    p.controllerAddress = controller.GetAddress();
    p.pawnHandle = pawnHandle;
    p.health = health;
    p.team = team;
    p.isTeammate = (localTeam != 0 && team == localTeam);
    p.position = position;

    // Read spotted state
    uint32_t spottedMask = pawn.GetSpottedStateMask();
    p.isSpotted = (spottedMask & (1 << localSlot)) != 0;

    // Name (Cached)
    if (s_nameCache.find(pawnHandle) == s_nameCache.end()) {
      s_nameCache[pawnHandle] = controller.GetPlayerName();
    }
    p.name = s_nameCache[pawnHandle];

    // Distance
    {
      float dx = position.x - localPos.x;
      float dy = position.y - localPos.y;
      float dz = position.z - localPos.z;
      p.distance = sqrtf(dx * dx + dy * dy + dz * dz) / 100.0f;
    }

    // Weapon
    if (s_readWeapons.load(std::memory_order_relaxed)) {
      p.weapon = pawn.GetWeaponName();
    }

    // ── Skeleton bones (one batch ReadRaw) ──
    if (s_readBones.load(std::memory_order_relaxed)) {
      uintptr_t gameScene = pawn.GetGameSceneNode();
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
    }

    players.emplace_back(std::move(p));
  }

  // ── Push to frontend (Thread-Safe Buffer) ──
  int writeIdx = activeBufferIndex.load(std::memory_order_relaxed) ^ 1;
  playerBuffers[writeIdx] = players;
  activeBufferIndex.store(writeIdx, std::memory_order_release);

  {
    std::unique_lock<std::shared_mutex> lock(stateMutex);
    cachedViewMatrix = viewMatrix;
    cachedLocalPos = localPos;
    cachedLocalEyePos = localEyePos;
    cachedLocalAngles = localAngles;
    cachedLocalAimPunch = localAimPunch;
    cachedLocalShotsFired = localShotsFired;
    cachedLocalTeam = localTeam;
    cachedLocalScoped = localScoped;
    cachedLocalCrosshairHandle = localCrosshairHandle;
    cachedLocalPawn = localPawn;
    cachedEntityList = entityList;
    cachedBombInfo = bombInfo;
  }
}

// ── Thread-safe rendering getters ─────────────────────────────────────────────

SDK::Matrix4x4 GameManager::GetViewMatrix() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedViewMatrix;
}

std::vector<SDK::Entity> GameManager::GetRenderPlayers() {
  int readIdx = activeBufferIndex.load(std::memory_order_acquire);
  return playerBuffers[readIdx];
}

SDK::Vector3 GameManager::GetLocalPos() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedLocalPos;
}

SDK::Vector3 GameManager::GetLocalEyePos() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedLocalEyePos;
}


SDK::Vector2 GameManager::GetLocalAimPunch() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedLocalAimPunch;
}

int GameManager::GetLocalShotsFired() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedLocalShotsFired;
}

int GameManager::GetLocalTeam() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedLocalTeam;
}

SDK::Vector2 GameManager::GetLocalAngles() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedLocalAngles;
}

bool GameManager::IsLocalScoped() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedLocalScoped;
}

uint32_t GameManager::GetLocalCrosshairEntityHandle() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedLocalCrosshairHandle;
}

SDK::BombInfo GameManager::GetBombInfo() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedBombInfo;
}

uintptr_t GameManager::GetLocalPlayerPawn() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedLocalPawn;
}

uintptr_t GameManager::GetEntityList() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return cachedEntityList;
}

// NOTE: Now protected by shared_lock — clientBase written once during Init()
uintptr_t GameManager::GetClientBase() {
  std::shared_lock<std::shared_mutex> lock(stateMutex);
  return clientBase;
}

// NOTE: Makes a live RPM call — must be called only from memory thread
uintptr_t GameManager::GetEntityFromHandle(uint32_t handle) {
  if (!handle || handle == 0xFFFFFFFF)
    return 0;

  uintptr_t list = GetEntityList();
  if (!list)
    return 0;

  uintptr_t listEntry = MemoryManager::Read<uintptr_t>(
      list + 0x10 + 0x8 * ((handle & 0x7FFF) >> 9));
  if (!listEntry)
    return 0;

  uintptr_t entity =
      MemoryManager::Read<uintptr_t>(listEntry + 0x70 * (handle & 0x1FF));
  return entity;
}

std::string GameManager::GetLocalWeaponName() {
  uintptr_t pawn = GetLocalPlayerPawn();
  if (!pawn || pawn <= 0x10000)
    return "";
  uintptr_t cw =
      MemoryManager::Read<uintptr_t>(pawn + SDK::Offsets::m_pClippingWeapon);
  if (cw <= 0x10000)
    return "";
  uintptr_t wPtr = MemoryManager::Read<uintptr_t>(cw + 0x10);
  if (wPtr <= 0x10000)
    return "";
  uintptr_t nPtr = MemoryManager::Read<uintptr_t>(wPtr + 0x20);
  if (nPtr <= 0x10000)
    return "";
  char wb[64] = {};
  MemoryManager::ReadRaw(nPtr, wb, sizeof(wb) - 1);
  std::string name(wb);
  if (name.rfind("weapon_", 0) == 0)
    return name.substr(7);
  return name;
}

uintptr_t GameManager::GetEntityGameSceneNode(uintptr_t entity) {
  if (!entity)
    return 0;
  return MemoryManager::Read<uintptr_t>(entity +
                                        SDK::Offsets::m_pGameSceneNode);
}

void GameManager::EnableBoneRead(bool enable) {
  s_readBones.store(enable, std::memory_order_relaxed);
}

void GameManager::EnableWeaponRead(bool enable) {
  s_readWeapons.store(enable, std::memory_order_relaxed);
}

} // namespace Core
