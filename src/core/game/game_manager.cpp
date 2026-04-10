#include "core/constants.h"
#include "game_manager.h"
#include "../memory/memory_manager.h"
#include "../process/module.h"
#include "../sdk/entity_classes.h"
#include "../sdk/offsets.h"
#include "config/settings.h"
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <unordered_map>

namespace Core {

namespace {

constexpr int LIST_CHUNKS = 64;
constexpr uint32_t INVALID_PAWN_HANDLE = 0xFFFFFFFFu;

uintptr_t s_pawnListCache[LIST_CHUNKS] = {};
std::unordered_map<uint32_t, std::string> s_nameCache;
std::unordered_map<uintptr_t, SDK::Vector3> s_prevPositions;

void InvalidateCachedEntityData() {
  std::memset(s_pawnListCache, 0, sizeof(s_pawnListCache));
  s_nameCache.clear();
  s_prevPositions.clear();
}

bool IsInvalidPawnHandle(uint32_t pawnHandle) {
  return pawnHandle == 0 || pawnHandle == INVALID_PAWN_HANDLE;
}

float ComputeDistanceSquared(const SDK::Vector3 &from, const SDK::Vector3 &to,
                             float &dx, float &dy, float &dz) {
  dx = to.x - from.x;
  dy = to.y - from.y;
  dz = to.z - from.z;
  return dx * dx + dy * dy + dz * dz;
}

} // namespace

std::shared_mutex GameManager::stateMutex;
SDK::Matrix4x4 GameManager::cachedViewMatrix = {};

std::vector<SDK::Entity> GameManager::playerBuffers[2];
std::atomic<int> GameManager::activeBufferIndex{0};
std::mutex GameManager::bufferMutex;

std::atomic<bool> GameManager::s_readBones{false};
std::atomic<bool> GameManager::s_readWeapons{false};
float GameManager::s_interpolationFactor{0.98f};
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

std::unordered_map<int, int> GameManager::s_invalidSlotCache;
int GameManager::s_screenWidth = 1920;
int GameManager::s_screenHeight = 1080;

bool GameManager::Init() {
  clientBase = Module::GetBaseAddress(L"client.dll");
  return clientBase != 0;
}

void GameManager::Update() {
  if (!clientBase && !Init()) {
    return;
  }

  viewMatrix =
      MemoryManager::Read<SDK::Matrix4x4>(clientBase + SDK::Offsets::dwViewMatrix);

  players.clear();
  players.reserve(16);

  FrameContext context{SDK::CEntityList(0), SDK::CPlayerPawn(0)};
  if (!BuildFrameContext(context)) {
    return;
  }

  ResetLocalState();
  UpdateLocalState(context.localPlayer);
  UpdateBombState();

  const int localSlot = FindLocalSlot(context);
  DecrementInvalidSlotCache();
  RebuildPlayers(context, localSlot);
  PublishFrameState();

#ifdef DEBUG
  static auto lastDebugTime = std::chrono::steady_clock::now();
  const auto now = std::chrono::steady_clock::now();
  if (std::chrono::duration_cast<std::chrono::seconds>(now - lastDebugTime)
          .count() >= 5) {
    lastDebugTime = now;

    std::cout << "[DEBUG] Entity List: 0x" << std::hex << entityList << std::dec
              << "\n";
    std::cout << "[DEBUG] Local Pawn: 0x" << std::hex << localPawn << std::dec
              << "\n";
    std::cout << "[DEBUG] Client Base: 0x" << std::hex << clientBase << std::dec
              << "\n";
    std::cout << "[DEBUG] Players found: " << players.size() << "\n";
    std::cout << "[DEBUG] Local Team: " << localTeam << "\n";
    std::cout << "[DEBUG] Local Pos: " << localPos.x << ", " << localPos.y
              << ", " << localPos.z << "\n";

    for (size_t i = 0; i < players.size(); ++i) {
      const auto &p = players[i];
      std::cout << "  Player[" << i << "]: addr=0x" << std::hex << p.address
                << " hp=" << std::dec << p.health << " team=" << p.team
                << " dist=" << p.distance << "m"
                << " name=" << p.name << " weapon=" << p.weapon
                << " spotted=" << p.isSpotted
                << " bones=" << p.bonePositions.size() << "\n";
    }
    std::cout << std::endl;
  }
#endif
}

void GameManager::ResetLocalState() {
  localPos = {};
  localEyePos = {};
  localAngles = {};
  localAimPunch = {};
  localShotsFired = 0;
  localTeam = 0;
  localScoped = false;
  localCrosshairHandle = 0;
  localPawn = 0;
}

void GameManager::UpdateLocalState(const SDK::CPlayerPawn &currentLocalPlayer) {
  localPawn = currentLocalPlayer.GetAddress();
  if (!currentLocalPlayer.IsValid()) {
    return;
  }

  localPos = currentLocalPlayer.GetOldOrigin();
  localEyePos = currentLocalPlayer.GetCameraPos();
  localTeam = currentLocalPlayer.GetTeam();
  localAngles = currentLocalPlayer.GetEyeAngles();
  localAimPunch = currentLocalPlayer.GetAimPunch();
  localShotsFired = currentLocalPlayer.GetShotsFired();
  localScoped = currentLocalPlayer.IsScoped();
  localCrosshairHandle = currentLocalPlayer.GetCrosshairEntityHandle();
}

void GameManager::UpdateBombState() {
  bombInfo = {};
  bombInfo.site = -1;

  if (SDK::Offsets::dwPlantedC4 == 0) {
    return;
  }

  uintptr_t c4Ptr =
      MemoryManager::Read<uintptr_t>(clientBase + SDK::Offsets::dwPlantedC4);
  if (c4Ptr <= Constants::MIN_VALID_ADDRESS) {
    return;
  }

  uintptr_t c4Addr = MemoryManager::Read<uintptr_t>(c4Ptr);
  if (c4Addr < Constants::MIN_VALID_ADDRESS) {
    c4Addr = c4Ptr;
  }

  SDK::CPlantedC4 c4(c4Addr);
  if (!c4.IsTicking()) {
    return;
  }

  bombInfo.isPlanted = true;
  bombInfo.site = c4.GetSite();
  bombInfo.timeLeft = c4.GetTimeLeft();
}

bool GameManager::BuildFrameContext(FrameContext &context) {
  entityList =
      MemoryManager::Read<uintptr_t>(clientBase + SDK::Offsets::dwEntityList);
  if (!entityList) {
    return false;
  }

  context.entityListObj = SDK::CEntityList(entityList);

  static uintptr_t s_lastFirstChunk = 0;
  const uintptr_t currentFirstChunk = context.entityListObj.GetListEntry(0);
  if (currentFirstChunk != s_lastFirstChunk) {
    s_lastFirstChunk = currentFirstChunk;
    InvalidateCachedEntityData();
  }

  context.localPlayer = SDK::CPlayerPawn(
      MemoryManager::Read<uintptr_t>(clientBase + SDK::Offsets::dwLocalPlayerPawn));

  uintptr_t listEntry = MemoryManager::Read<uintptr_t>(
      entityList + Constants::ENTITY_LIST_HEADER_OFFSET);
  if (!listEntry) {
    return false;
  }

  std::memset(s_pawnListCache, 0, sizeof(s_pawnListCache));
  for (int i = 0; i < Constants::MAX_PLAYERS; ++i) {
    context.controllerPointers[i] = MemoryManager::Read<uintptr_t>(
        listEntry + (i + 1) * Constants::ENTITY_IDENTITY_ENTRY_SIZE);
  }

  return true;
}

int GameManager::FindLocalSlot(const FrameContext &context) {
  for (uintptr_t controllerPtr : context.controllerPointers) {
    SDK::CPlayerController controller(controllerPtr);
    if (!controller.IsValid()) {
      continue;
    }

    const uint32_t pawnHandle = controller.GetPawnHandle();
    if (!IsInvalidPawnHandle(pawnHandle) && controller.IsLocalPlayerController()) {
      return static_cast<int>((pawnHandle & 0x7FFF) - 1);
    }
  }

  return -1;
}

void GameManager::DecrementInvalidSlotCache() {
  for (auto it = s_invalidSlotCache.begin(); it != s_invalidSlotCache.end();) {
    --it->second;
    if (it->second <= 0) {
      it = s_invalidSlotCache.erase(it);
    } else {
      ++it;
    }
  }
}

void GameManager::RebuildPlayers(const FrameContext &context, int localSlot) {
  constexpr float maxDistSq =
      Constants::ESP_MAX_DISTANCE_UNITS * Constants::ESP_MAX_DISTANCE_UNITS;

  for (uintptr_t controllerPtr : context.controllerPointers) {
    SDK::CPlayerController controller(controllerPtr);
    if (!controller.IsValid()) {
      continue;
    }

    const uint32_t pawnHandle = controller.GetPawnHandle();
    if (IsInvalidPawnHandle(pawnHandle)) {
      continue;
    }

    const int slot = static_cast<int>((pawnHandle & 0x7FFF) - 1);
    const auto invalidIt = s_invalidSlotCache.find(slot);
    if (invalidIt != s_invalidSlotCache.end() && invalidIt->second > 0) {
      continue;
    }

    const int chunkIdx = static_cast<int>((pawnHandle & 0x7FFF) >> 9);
    if (chunkIdx < 0 || chunkIdx >= LIST_CHUNKS) {
      continue;
    }

    if (!s_pawnListCache[chunkIdx]) {
      s_pawnListCache[chunkIdx] = context.entityListObj.GetListEntry(chunkIdx);
    }

    SDK::CPlayerPawn pawn =
        context.entityListObj.GetPawnFromHandle(pawnHandle, s_pawnListCache[chunkIdx]);
    if (!pawn.IsValid() || pawn.GetAddress() == localPawn) {
      s_invalidSlotCache[slot] = INVALID_SLOT_SKIP_FRAMES;
      continue;
    }

    const int health = pawn.GetHealth();
    if (health <= 0 || health > 100) {
      s_invalidSlotCache[slot] = INVALID_SLOT_SKIP_FRAMES;
      continue;
    }

    const int team = pawn.GetTeam();
    if (team != 2 && team != 3) {
      s_invalidSlotCache[slot] = INVALID_SLOT_SKIP_FRAMES;
      continue;
    }

    const SDK::Vector3 position = pawn.GetOldOrigin();
    if (position.x == 0.0f && position.y == 0.0f && position.z == 0.0f) {
      s_invalidSlotCache[slot] = INVALID_SLOT_SKIP_FRAMES;
      continue;
    }

    float dx = 0.0f;
    float dy = 0.0f;
    float dz = 0.0f;
    const float distSq = ComputeDistanceSquared(localPos, position, dx, dy, dz);
    const bool tooFar = distSq > maxDistSq;

    SDK::Entity entity;
    entity.address = pawn.GetAddress();
    entity.controllerAddress = controller.GetAddress();
    entity.pawnHandle = pawnHandle;
    entity.health = health;
    entity.team = team;
    entity.isTeammate = (localTeam != 0 && team == localTeam);
    entity.position = position;

    const float interpFactor = s_interpolationFactor;
    const auto prevIt = s_prevPositions.find(entity.address);
    if (prevIt != s_prevPositions.end()) {
      entity.prevPosition = prevIt->second;
      entity.renderPosition.x =
          entity.prevPosition.x + (position.x - entity.prevPosition.x) * interpFactor;
      entity.renderPosition.y =
          entity.prevPosition.y + (position.y - entity.prevPosition.y) * interpFactor;
      entity.renderPosition.z =
          entity.prevPosition.z + (position.z - entity.prevPosition.z) * interpFactor;
      entity.interpolationFactor = interpFactor;
    } else {
      entity.prevPosition = position;
      entity.renderPosition = position;
      entity.interpolationFactor = 1.0f;
    }
    s_prevPositions[entity.address] = position;

    if (s_prevPositions.size() > Constants::MAX_POSITION_CACHE_SIZE) {
      const size_t toRemove = s_prevPositions.size() / 2;
      for (size_t i = 0; i < toRemove && !s_prevPositions.empty(); ++i) {
        s_prevPositions.erase(s_prevPositions.begin());
      }
    }

    const uint32_t spottedMask = pawn.GetSpottedStateMask();
    entity.isSpotted =
        localSlot >= 0 && (spottedMask & (1u << localSlot)) != 0;

    if (s_nameCache.find(pawnHandle) == s_nameCache.end()) {
      s_nameCache[pawnHandle] = controller.GetPlayerName();
    }
    entity.name = s_nameCache[pawnHandle];

    entity.distance = std::sqrt(distSq) / 100.0f;

    if (!tooFar && s_readWeapons.load(std::memory_order_relaxed)) {
      entity.weapon = pawn.GetWeaponName();
    }

    if (!tooFar && s_readBones.load(std::memory_order_relaxed)) {
      const uintptr_t gameScene = pawn.GetGameSceneNode();
      if (gameScene > Constants::MIN_VALID_ADDRESS) {
        const uintptr_t boneArray = MemoryManager::Read<uintptr_t>(
            gameScene + SDK::Offsets::m_boneArrayOffset);
        if (boneArray > Constants::MIN_VALID_ADDRESS) {
          BoneData rawBones[BONE_COUNT];
          if (MemoryManager::ReadRaw(boneArray, rawBones, sizeof(rawBones))) {
            entity.bonePositions.resize(BONE_COUNT);
            for (int boneIndex = 0; boneIndex < BONE_COUNT; ++boneIndex) {
              entity.bonePositions[boneIndex] = rawBones[boneIndex].pos;
            }
          }
        }
      }
    }

    entity.onScreen = IsOnScreen(position);
    players.emplace_back(std::move(entity));
  }
}

void GameManager::PublishFrameState() {
  const int writeIdx = activeBufferIndex.load(std::memory_order_relaxed) ^ 1;
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
}

void GameManager::EnableBoneRead(bool enable) {
  s_readBones.store(enable, std::memory_order_relaxed);
}

void GameManager::EnableWeaponRead(bool enable) {
  s_readWeapons.store(enable, std::memory_order_relaxed);
}

void GameManager::SetInterpolationFactor(float factor) {
  s_interpolationFactor = factor;
}

void GameManager::SetScreenSize(int width, int height) {
  s_screenWidth = width;
  s_screenHeight = height;
}

bool GameManager::IsOnScreen(const SDK::Vector3 &worldPos) {
  const SDK::Matrix4x4 vm = viewMatrix;
  const float clipX = vm.m[0][0] * worldPos.x + vm.m[0][1] * worldPos.y +
                      vm.m[0][2] * worldPos.z + vm.m[0][3];
  const float clipY = vm.m[1][0] * worldPos.x + vm.m[1][1] * worldPos.y +
                      vm.m[1][2] * worldPos.z + vm.m[1][3];
  const float clipW = vm.m[3][0] * worldPos.x + vm.m[3][1] * worldPos.y +
                      vm.m[3][2] * worldPos.z + vm.m[3][3];

  if (clipW < 0.001f) {
    return false;
  }

  const float ndcX = clipX / clipW;
  const float ndcY = clipY / clipW;
  return (ndcX >= -1.0f - FRUSTUM_MARGIN / s_screenWidth &&
          ndcX <= 1.0f + FRUSTUM_MARGIN / s_screenWidth &&
          ndcY >= -1.0f - FRUSTUM_MARGIN / s_screenHeight &&
          ndcY <= 1.0f + FRUSTUM_MARGIN / s_screenHeight);
}

} // namespace Core
