#include "game_manager.h"
#include "../memory/memory_manager.h"
#include "../process/module.h"
#include "../sdk/offsets.h"
#include "../sdk/entity_classes.h"
#include <cmath>
#include <unordered_map>
#include <iostream>
#include <chrono>

namespace Core {

// Thread synchronization primitives
std::shared_mutex GameManager::stateMutex;
SDK::Matrix4x4 GameManager::cachedViewMatrix = {};

std::vector<SDK::Entity> GameManager::playerBuffers[2];
std::atomic<int> GameManager::activeBufferIndex{0};
std::mutex GameManager::bufferMutex;

std::atomic<bool> GameManager::s_readBones{false};
std::atomic<bool> GameManager::s_readWeapons{false};
float GameManager::s_interpolationFactor{0.98f}; // Higher = closer to current position, less ghosting (tuned for responsive ESP)
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

// Position interpolation cache: entity address -> previous position
static std::unordered_map<uintptr_t, SDK::Vector3> s_prevPositions;

// Invalid slot cache implementation
std::unordered_map<int, int> GameManager::s_invalidSlotCache;

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
    s_prevPositions.clear();
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

  // ── Decrement invalid slot cache counters ──
  for (auto it = s_invalidSlotCache.begin(); it != s_invalidSlotCache.end(); ) {
    it->second--;
    if (it->second <= 0) {
      it = s_invalidSlotCache.erase(it);
    } else {
      ++it;
    }
  }

  for (int i = 0; i < MAX_PLAYERS; i++) {
    SDK::CPlayerController controller(controllerPointers[i]);
    if (!controller.IsValid())
      continue;

    uint32_t pawnHandle = controller.GetPawnHandle();
    if (!pawnHandle || pawnHandle == 0xFFFFFFFF)
      continue;

    // Check invalid slot cache
    int slot = (pawnHandle & 0x7FFF) - 1;
    auto cacheIt = s_invalidSlotCache.find(slot);
    if (cacheIt != s_invalidSlotCache.end() && cacheIt->second > 0) {
      continue; // Skip reading this slot for N frames
    }

    // Cached pawn list chunk lookup
    int chunkIdx = (int)((pawnHandle & 0x7FFF) >> 9);
    if (chunkIdx < 0 || chunkIdx >= LIST_CHUNKS)
      continue;

    if (!s_pawnListCache[chunkIdx]) {
      s_pawnListCache[chunkIdx] = entityListObj.GetListEntry(chunkIdx);
    }
    
    SDK::CPlayerPawn pawn = entityListObj.GetPawnFromHandle(pawnHandle, s_pawnListCache[chunkIdx]);
    if (!pawn.IsValid() || pawn.GetAddress() == localPawn) {
      // Add to invalid cache
      s_invalidSlotCache[slot] = INVALID_SLOT_SKIP_FRAMES;
      continue;
    }

    int health = pawn.GetHealth();
    if (health <= 0 || health > 100) {
      s_invalidSlotCache[slot] = INVALID_SLOT_SKIP_FRAMES;
      continue;
    }

    int team = pawn.GetTeam();
    if (team != 2 && team != 3) {
      s_invalidSlotCache[slot] = INVALID_SLOT_SKIP_FRAMES;
      continue;
    }

    SDK::Vector3 position = pawn.GetOldOrigin();
    if (position.x == 0.0f && position.y == 0.0f && position.z == 0.0f) {
      s_invalidSlotCache[slot] = INVALID_SLOT_SKIP_FRAMES;
      continue;
    }

    // ── Early distance check (skip distant entities before expensive reads) ──
    float dx = position.x - localPos.x;
    float dy = position.y - localPos.y;
    float dz = position.z - localPos.z;
    float distSq = dx * dx + dy * dy + dz * dz;
    
    // ESP max distance check (5000 units = 50m, squared = 25,000,000)
    constexpr float maxDistSq = 5000.0f * 5000.0f;
    bool tooFar = distSq > maxDistSq;
    
    // If entity is too far, skip expensive reads (bones/weapons)
    // but still process the entity for basic ESP (box, health, name, distance)
    if (tooFar) {
      // Skip expensive bone/weapon reads for distant entities
      // but don't add to invalid cache — we still need the entity for ESP
      // Just skip the expensive reads below
    }

    // ── Build entity ──
    SDK::Entity p;
    p.address = pawn.GetAddress();
    p.controllerAddress = controller.GetAddress();
    p.pawnHandle = pawnHandle;
    p.health = health;
    p.team = team;
    p.isTeammate = (localTeam != 0 && team == localTeam);
    p.position = position;

    // ── Position interpolation ──
    float interpFactor = s_interpolationFactor;
    auto prevIt = s_prevPositions.find(p.address);
    if (prevIt != s_prevPositions.end()) {
      p.prevPosition = prevIt->second;
      // Lerp: renderPosition = prevPosition + (position - prevPosition) * factor
      p.renderPosition.x = p.prevPosition.x + (position.x - p.prevPosition.x) * interpFactor;
      p.renderPosition.y = p.prevPosition.y + (position.y - p.prevPosition.y) * interpFactor;
      p.renderPosition.z = p.prevPosition.z + (position.z - p.prevPosition.z) * interpFactor;
      p.interpolationFactor = interpFactor;
    } else {
      // First frame: no interpolation
      p.prevPosition = position;
      p.renderPosition = position;
      p.interpolationFactor = 1.0f;
    }
    // Update previous position for next frame
    s_prevPositions[p.address] = position;

    // Ограничиваем размер кэша (макс 128 сущностей)
    if (s_prevPositions.size() > 128) {
      // Clear half of entries (unordered_map has no ordering, so we clear arbitrary entries)
      size_t toRemove = s_prevPositions.size() / 2;
      for (size_t i = 0; i < toRemove && !s_prevPositions.empty(); ++i) {
        s_prevPositions.erase(s_prevPositions.begin());
      }
    }

    // Read spotted state
    uint32_t spottedMask = pawn.GetSpottedStateMask();
    p.isSpotted = (spottedMask & (1 << localSlot)) != 0;

    // Name (Cached)
    if (s_nameCache.find(pawnHandle) == s_nameCache.end()) {
      s_nameCache[pawnHandle] = controller.GetPlayerName();
    }
    p.name = s_nameCache[pawnHandle];

    // Distance (reuse dx, dy, dz computed earlier)
    p.distance = sqrtf(distSq) / 100.0f;

    // Weapon — skip for distant entities
    if (!tooFar && s_readWeapons.load(std::memory_order_relaxed)) {
      p.weapon = pawn.GetWeaponName();
    }

    // ── Skeleton bones (one batch ReadRaw) — skip for distant entities ──
    if (!tooFar && s_readBones.load(std::memory_order_relaxed)) {
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
  {
    std::lock_guard<std::mutex> lock(bufferMutex);
    playerBuffers[writeIdx] = players;
  }
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

  // Debug output every 5 seconds
#ifdef DEBUG
  static auto lastDebugTime = std::chrono::steady_clock::now();
  auto now = std::chrono::steady_clock::now();
  if (std::chrono::duration_cast<std::chrono::seconds>(now - lastDebugTime).count() >= 5) {
    lastDebugTime = now;
    
    std::cout << "[DEBUG] Entity List: 0x" << std::hex << entityList << std::dec << "\n";
    std::cout << "[DEBUG] Local Pawn: 0x" << std::hex << localPawn << std::dec << "\n";
    std::cout << "[DEBUG] Client Base: 0x" << std::hex << clientBase << std::dec << "\n";
    std::cout << "[DEBUG] Players found: " << players.size() << "\n";
    std::cout << "[DEBUG] Local Team: " << localTeam << "\n";
    std::cout << "[DEBUG] Local Pos: " << localPos.x << ", " << localPos.y << ", " << localPos.z << "\n";
    
    for (size_t i = 0; i < players.size(); i++) {
      const auto& p = players[i];
      std::cout << "  Player[" << i << "]: addr=0x" << std::hex << p.address 
                << " hp=" << std::dec << p.health 
                << " team=" << p.team 
                << " dist=" << p.distance << "m"
                << " name=" << p.name
                << " weapon=" << p.weapon
                << " spotted=" << p.isSpotted
                << " bones=" << p.bonePositions.size() << "\n";
    }
    std::cout << std::endl;
  }
#endif
}

void GameManager::EnableBoneRead(bool enable) {
  s_readBones.store(enable, std::memory_order_relaxed);
}

void GameManager::EnableWeaponRead(bool enable) {
  s_readWeapons.store(enable, std::memory_order_relaxed);
}

void GameManager::SetInterpolationFactor(float factor) {
  if (factor >= 0.0f && factor <= 1.0f) {
    s_interpolationFactor = factor;
  }
}

} // namespace Core
