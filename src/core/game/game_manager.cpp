#include "core/constants.h"
#include "game_manager.h"
#include "../memory/memory_manager.h"
#include "../process/module.h"
#include "../process/process.h"
#include "../sdk/entity_classes.h"
#include "../sdk/offsets.h"
#include "config/settings.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <deque>
#include <iostream>
#include <limits>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace Core {

namespace {

constexpr int LIST_CHUNKS = 64;
constexpr uint32_t INVALID_PAWN_HANDLE = 0xFFFFFFFFu;
constexpr float kTraceFallbackRange = 2200.0f;
constexpr float kTraceMinRange = 320.0f;
constexpr float kTraceMaxRange = 4096.0f;
constexpr float kTraceRetentionSeconds = 2.5f;
constexpr float kShotRetentionSeconds = 2.5f;
constexpr float kHitRetentionSeconds = 1.2f;
constexpr float kAudioRetentionSeconds = 2.5f;
constexpr float kHitEventWindowSeconds = 0.75f;
constexpr float kImpactMatchDistanceSq = 96.0f * 96.0f;
constexpr float kImpactLooseMatchDistanceSq = 160.0f * 160.0f;
constexpr float kPendingImpactMatchWindowSeconds = 0.80f;
constexpr float kFallbackTraceDelaySeconds = 0.06f;
constexpr float kPendingShotExpireSeconds = 0.85f;
constexpr float kSeenImpactRetentionSeconds = 3.0f;
constexpr float kTeleportResetDistance = 128.0f;
constexpr float kFootstepSpeedThreshold = 70.0f;
constexpr float kFootstepMinDistance = 20.0f;
constexpr float kFootstepMaxDistance = 44.0f;

using TelemetryClock = std::chrono::steady_clock;

uintptr_t s_pawnListCache[LIST_CHUNKS] = {};
std::unordered_map<uint32_t, std::string> s_nameCache;
std::unordered_map<uintptr_t, SDK::Vector3> s_prevPositions;
TelemetryClock::time_point s_telemetryStart = TelemetryClock::now();

struct PendingShot {
  uint64_t shotId = 0;
  int shotsFired = 0;
  SDK::Vector3 start = {};
  SDK::Vector3 predictedEnd = {};
  SDK::Vector2 angle = {};
  float timeSeconds = 0.0f;
  bool fallbackEmitted = false;
  bool impactMatched = false;
};

struct MovementEventState {
  bool initialized = false;
  bool onGround = false;
  float horizontalSpeed = 0.0f;
  SDK::Vector3 position = {};
  SDK::Vector3 lastStepPosition = {};
  float accumulatedGroundDistance = 0.0f;
  float airborneStartZ = 0.0f;
  float lastFootstepTime = -100.0f;
  float lastJumpTime = -100.0f;
  float lastLandTime = -100.0f;
};

uint64_t s_nextTelemetryId = 1;
uintptr_t s_telemetryLocalPawn = 0;
int s_prevLocalShotsFired = 0;
std::unordered_set<uint64_t> s_seenImpactKeys;
std::deque<std::pair<uint64_t, float>> s_seenImpactHistory;
std::deque<PendingShot> s_pendingShots;
std::unordered_map<uintptr_t, int> s_lastEnemyHealth;
std::unordered_map<uint32_t, MovementEventState> s_movementStates;
std::vector<SDK::ShotEvent> s_recentShotEvents;
std::vector<SDK::BulletTraceEvent> s_recentTraceEvents;
std::vector<SDK::HitEvent> s_recentHitEvents;
std::vector<SDK::MovementAudioEvent> s_recentAudioEvents;

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

float GetFrameTimeSeconds() {
  return std::chrono::duration<float>(TelemetryClock::now() - s_telemetryStart)
      .count();
}

uint64_t NextTelemetryId() {
  return s_nextTelemetryId++;
}

SDK::Vector3 ForwardFromAngles(const SDK::Vector2 &angles) {
  constexpr float kDegToRad = 3.14159265f / 180.0f;
  const float pitch = angles.x * kDegToRad;
  const float yaw = angles.y * kDegToRad;
  const float cosPitch = std::cos(pitch);
  return {cosPitch * std::cos(yaw), cosPitch * std::sin(yaw), -std::sin(pitch)};
}

float NormalizeAngleDegrees(float angle) {
  while (angle > 180.0f) {
    angle -= 360.0f;
  }
  while (angle < -180.0f) {
    angle += 360.0f;
  }
  return angle;
}

bool IsFiniteAngleSet(const SDK::Vector2 &angles) {
  return std::isfinite(angles.x) && std::isfinite(angles.y);
}

SDK::Vector2 SanitizeAngles(const SDK::Vector2 &angles) {
  SDK::Vector2 sanitized = angles;
  sanitized.x = std::clamp(NormalizeAngleDegrees(sanitized.x), -89.0f, 89.0f);
  sanitized.y = NormalizeAngleDegrees(sanitized.y);
  return sanitized;
}

SDK::Vector2 ChooseTracerAngles(const SDK::Vector2 &eyeAngles,
                                const SDK::Vector2 &shootAngles) {
  const SDK::Vector2 fallback = SanitizeAngles(eyeAngles);
  if (!IsFiniteAngleSet(shootAngles) ||
      (shootAngles.x == 0.0f && shootAngles.y == 0.0f)) {
    return fallback;
  }

  const SDK::Vector2 candidate = SanitizeAngles(shootAngles);
  const float pitchDelta = std::abs(candidate.x - fallback.x);
  const float yawDelta =
      std::abs(NormalizeAngleDegrees(candidate.y - fallback.y));

  if (pitchDelta > 12.0f || yawDelta > 20.0f) {
    return fallback;
  }

  return candidate;
}

uint64_t MakeImpactKey(const SDK::BulletImpactInfo &impact) {
  const int px = static_cast<int>(std::round(impact.position.x * 0.25f));
  const int py = static_cast<int>(std::round(impact.position.y * 0.25f));
  const int pz = static_cast<int>(std::round(impact.position.z * 0.25f));
  const int ts = static_cast<int>(std::round(impact.timestamp * 1000.0f));
  const int exp = static_cast<int>(std::round(impact.expireTime * 1000.0f));

  const uint64_t ux = static_cast<uint32_t>(px);
  const uint64_t uy = static_cast<uint32_t>(py);
  const uint64_t uz = static_cast<uint32_t>(pz);
  const uint64_t uts = static_cast<uint32_t>(ts);
  const uint64_t uexp = static_cast<uint32_t>(exp);
  return (ux << 32) ^ (uy << 1) ^ (uz << 17) ^ (uts << 7) ^ (uexp << 21);
}

float HorizontalDistance(const SDK::Vector3 &a, const SDK::Vector3 &b) {
  const float dx = a.x - b.x;
  const float dy = a.y - b.y;
  return std::sqrt(dx * dx + dy * dy);
}

float DistanceSquared(const SDK::Vector3 &a, const SDK::Vector3 &b) {
  const float dx = a.x - b.x;
  const float dy = a.y - b.y;
  const float dz = a.z - b.z;
  return dx * dx + dy * dy + dz * dz;
}

float PointToSegmentDistanceSquared(const SDK::Vector3 &point,
                                    const SDK::Vector3 &segmentStart,
                                    const SDK::Vector3 &segmentEnd) {
  const float vx = segmentEnd.x - segmentStart.x;
  const float vy = segmentEnd.y - segmentStart.y;
  const float vz = segmentEnd.z - segmentStart.z;
  const float wx = point.x - segmentStart.x;
  const float wy = point.y - segmentStart.y;
  const float wz = point.z - segmentStart.z;

  const float segmentLengthSq = vx * vx + vy * vy + vz * vz;
  if (segmentLengthSq <= 0.0001f) {
    return DistanceSquared(point, segmentStart);
  }

  const float t =
      std::clamp((wx * vx + wy * vy + wz * vz) / segmentLengthSq, 0.0f, 1.0f);
  const SDK::Vector3 closest = {segmentStart.x + vx * t, segmentStart.y + vy * t,
                                segmentStart.z + vz * t};
  return DistanceSquared(point, closest);
}

float ComputeStepDistance(float horizontalSpeed) {
  const float normalized =
      std::clamp((horizontalSpeed - kFootstepSpeedThreshold) / 220.0f, 0.0f, 1.0f);
  return kFootstepMaxDistance -
         (kFootstepMaxDistance - kFootstepMinDistance) * normalized;
}

void PruneSeenImpacts(float nowSeconds) {
  while (!s_seenImpactHistory.empty() &&
         nowSeconds - s_seenImpactHistory.front().second >
             kSeenImpactRetentionSeconds) {
    s_seenImpactKeys.erase(s_seenImpactHistory.front().first);
    s_seenImpactHistory.pop_front();
  }
}

PendingShot *FindBestPendingShotForImpact(const SDK::Vector3 &impactPosition,
                                          float nowSeconds) {
  PendingShot *bestMatch = nullptr;
  float bestScore = std::numeric_limits<float>::max();

  for (auto &pending : s_pendingShots) {
    if (pending.impactMatched) {
      continue;
    }

    const float age = nowSeconds - pending.timeSeconds;
    if (age < 0.0f || age > kPendingImpactMatchWindowSeconds) {
      continue;
    }

    const float rayDistanceSq = PointToSegmentDistanceSquared(
        impactPosition, pending.start, pending.predictedEnd);
    const float score = rayDistanceSq + age * 4500.0f +
                        (pending.fallbackEmitted ? 500.0f : 0.0f);
    if (score < bestScore) {
      bestScore = score;
      bestMatch = &pending;
    }
  }

  return bestMatch;
}

SDK::Vector3 BuildApproxHitPosition(const SDK::Entity &player) {
  return {player.renderPosition.x, player.renderPosition.y,
          player.renderPosition.z + 42.0f};
}

void ResetTelemetryState() {
  s_telemetryLocalPawn = 0;
  s_prevLocalShotsFired = 0;
  s_seenImpactKeys.clear();
  s_seenImpactHistory.clear();
  s_pendingShots.clear();
  s_lastEnemyHealth.clear();
  s_movementStates.clear();
  s_recentShotEvents.clear();
  s_recentTraceEvents.clear();
  s_recentHitEvents.clear();
  s_recentAudioEvents.clear();
}

template <typename T>
void PruneRecentEvents(std::vector<T> &events, float cutoffTimeSeconds) {
  events.erase(std::remove_if(events.begin(), events.end(),
                              [cutoffTimeSeconds](const T &event) {
                                return event.timeSeconds < cutoffTimeSeconds;
                              }),
               events.end());
}

void PruneCaches(const std::unordered_set<uintptr_t> &activeAddresses,
                 const std::unordered_set<uint32_t> &activeHandles) {
  for (auto it = s_prevPositions.begin(); it != s_prevPositions.end();) {
    if (activeAddresses.find(it->first) == activeAddresses.end()) {
      it = s_prevPositions.erase(it);
    } else {
      ++it;
    }
  }

  for (auto it = s_nameCache.begin(); it != s_nameCache.end();) {
    if (activeHandles.find(it->first) == activeHandles.end()) {
      it = s_nameCache.erase(it);
    } else {
      ++it;
    }
  }
}

} // namespace

std::atomic<std::shared_ptr<const GameSnapshot>> GameManager::s_snapshot{
    std::make_shared<GameSnapshot>()};
std::atomic<bool> GameManager::s_readBones{false};
std::atomic<bool> GameManager::s_readWeapons{false};
std::atomic<float> GameManager::s_interpolationFactor{0.98f};
std::atomic<int> GameManager::s_screenWidth{1920};
std::atomic<int> GameManager::s_screenHeight{1080};
std::unordered_map<int, int> GameManager::s_invalidSlotCache;

uintptr_t GameManager::clientBase = 0;
SDK::Matrix4x4 GameManager::viewMatrix = {};
std::vector<SDK::Entity> GameManager::players;
SDK::Vector3 GameManager::localPos = {};
SDK::Vector3 GameManager::localEyePos = {};
SDK::Vector2 GameManager::localAngles = {};
SDK::Vector2 GameManager::localShootAngle = {};
SDK::Vector2 GameManager::localAimPunch = {};
int GameManager::localShotsFired = 0;
int GameManager::localTeam = 0;
bool GameManager::localScoped = false;
uint32_t GameManager::localCrosshairHandle = 0;
uintptr_t GameManager::localPawn = 0;
uintptr_t GameManager::entityList = 0;
SDK::BombInfo GameManager::bombInfo = {};
std::string GameManager::localWeaponName;
float GameManager::localWeaponRange = 0.0f;
std::vector<SDK::BulletImpactInfo> GameManager::localBulletImpacts;
float GameManager::frameTimeSeconds = 0.0f;
std::vector<SDK::ShotEvent> GameManager::shotEvents;
std::vector<SDK::BulletTraceEvent> GameManager::bulletTraceEvents;
std::vector<SDK::HitEvent> GameManager::hitEvents;
std::vector<SDK::MovementAudioEvent> GameManager::movementAudioEvents;

bool GameManager::Init() {
  clientBase = Module::GetBaseAddress(L"client.dll");
  return clientBase != 0;
}

void GameManager::Update() {
  if (Process::GetProcessId() == 0) {
    ClearFrameState(true);
    return;
  }

  if (!clientBase && !Init()) {
    ClearFrameState(true);
    return;
  }

  const SDK::OffsetSet offsets = SDK::Offsets::GetCopy();
  if (!offsets.HasRequired()) {
    ClearFrameState(false);
    return;
  }

  frameTimeSeconds = GetFrameTimeSeconds();

  viewMatrix = MemoryManager::Read<SDK::Matrix4x4>(clientBase + offsets.dwViewMatrix);

  players.clear();
  players.reserve(16);

  FrameContext context{SDK::CEntityList(0, &offsets), SDK::CPlayerPawn(0, &offsets),
                       &offsets};
  if (!BuildFrameContext(context, offsets)) {
    ClearFrameState(false);
    return;
  }

  ResetLocalState();
  UpdateLocalState(context.localPlayer, offsets);
  UpdateBombState(context.localPlayer, offsets);

  const int localSlot = FindLocalSlot(context);
  DecrementInvalidSlotCache();
  RebuildPlayers(context, localSlot, offsets);
  UpdateCombatTelemetry();

  std::unordered_set<uintptr_t> activeAddresses;
  std::unordered_set<uint32_t> activeHandles;
  activeAddresses.reserve(players.size());
  activeHandles.reserve(players.size());
  for (const auto &player : players) {
    activeAddresses.insert(player.address);
    activeHandles.insert(player.pawnHandle);
  }
  PruneCaches(activeAddresses, activeHandles);

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
  }
#endif
}

void GameManager::ClearFrameState(bool clearClientBase) {
  viewMatrix = {};
  players.clear();
  ResetLocalState();
  entityList = 0;
  bombInfo = {};
  frameTimeSeconds = 0.0f;
  shotEvents.clear();
  bulletTraceEvents.clear();
  hitEvents.clear();
  movementAudioEvents.clear();

  if (clearClientBase) {
    clientBase = 0;
    s_invalidSlotCache.clear();
    InvalidateCachedEntityData();
  }

  ResetTelemetryState();

  PublishFrameState();
}

void GameManager::ResetLocalState() {
  localPos = {};
  localEyePos = {};
  localAngles = {};
  localShootAngle = {};
  localAimPunch = {};
  localShotsFired = 0;
  localTeam = 0;
  localScoped = false;
  localCrosshairHandle = 0;
  localPawn = 0;
  localWeaponName.clear();
  localWeaponRange = 0.0f;
  localBulletImpacts.clear();
}

void GameManager::UpdateLocalState(const SDK::CPlayerPawn &currentLocalPlayer,
                                   const SDK::OffsetSet &offsets) {
  localPawn = currentLocalPlayer.GetAddress();
  if (!currentLocalPlayer.IsValid()) {
    return;
  }

  localPos = currentLocalPlayer.GetOldOrigin();
  localEyePos = currentLocalPlayer.GetCameraPos();
  localTeam = currentLocalPlayer.GetTeam();
  localAngles = currentLocalPlayer.GetEyeAngles();
  localShootAngle = currentLocalPlayer.GetShootAngle();
  localAimPunch = currentLocalPlayer.GetAimPunch();
  localShotsFired = currentLocalPlayer.GetShotsFired();
  localScoped = currentLocalPlayer.IsScoped();
  localCrosshairHandle = currentLocalPlayer.GetCrosshairEntityHandle();
  localWeaponRange = currentLocalPlayer.GetWeaponRange();
  localBulletImpacts = currentLocalPlayer.GetBulletImpacts();

  uintptr_t clippingWeapon =
      MemoryManager::Read<uintptr_t>(currentLocalPlayer.GetAddress() + offsets.m_pClippingWeapon);
  if (clippingWeapon <= Constants::MIN_VALID_ADDRESS) {
    localWeaponName.clear();
    return;
  }

  uintptr_t weaponPtr = MemoryManager::Read<uintptr_t>(clippingWeapon + 0x10);
  uintptr_t namePtr = weaponPtr > Constants::MIN_VALID_ADDRESS
                          ? MemoryManager::Read<uintptr_t>(weaponPtr + 0x20)
                          : 0;
  if (namePtr <= Constants::MIN_VALID_ADDRESS) {
    localWeaponName.clear();
    return;
  }

  char weaponBuffer[64] = {};
  MemoryManager::ReadRaw(namePtr, weaponBuffer, sizeof(weaponBuffer) - 1);
  localWeaponName = weaponBuffer;
  if (localWeaponName.rfind("weapon_", 0) == 0) {
    localWeaponName = localWeaponName.substr(7);
  }
}

void GameManager::UpdateBombState(const SDK::CPlayerPawn &currentLocalPlayer,
                                  const SDK::OffsetSet &offsets) {
  bombInfo = {};
  bombInfo.site = -1;

  if (offsets.dwPlantedC4 == 0) {
    return;
  }

  uintptr_t c4Ptr = MemoryManager::Read<uintptr_t>(clientBase + offsets.dwPlantedC4);
  if (c4Ptr <= Constants::MIN_VALID_ADDRESS) {
    return;
  }

  uintptr_t c4Addr = MemoryManager::Read<uintptr_t>(c4Ptr);
  if (c4Addr < Constants::MIN_VALID_ADDRESS) {
    c4Addr = c4Ptr;
  }

  SDK::CPlantedC4 c4(c4Addr, &offsets);
  if (!c4.IsTicking()) {
    return;
  }

  bombInfo.isPlanted = true;
  bombInfo.site = c4.GetSite();
  bombInfo.totalTime = c4.GetTimerLength();
  if (bombInfo.totalTime <= 0.0f) {
    bombInfo.totalTime = 40.0f;
  }

  const float currentGameTime =
      currentLocalPlayer.IsValid() ? currentLocalPlayer.GetSimulationTime() : 0.0f;
  const float blowTime = c4.GetBlowTime();
  if (currentGameTime > 0.0f && blowTime > currentGameTime) {
    bombInfo.timeLeft = std::max(0.0f, blowTime - currentGameTime);
  } else {
    bombInfo.timeLeft = bombInfo.totalTime;
  }

  bombInfo.isBeingDefused = c4.IsBeingDefused();
  if (bombInfo.isBeingDefused) {
    const float defuseEndTime = c4.GetDefuseCountDown();
    if (currentGameTime > 0.0f && defuseEndTime > currentGameTime) {
      bombInfo.defuseTimeLeft = std::max(0.0f, defuseEndTime - currentGameTime);
    }
  }
}

void GameManager::UpdateCombatTelemetry() {
  if (localPawn == 0) {
    ResetTelemetryState();
    shotEvents.clear();
    bulletTraceEvents.clear();
    hitEvents.clear();
    movementAudioEvents.clear();
    return;
  }

  if (localPawn != s_telemetryLocalPawn || localShotsFired < s_prevLocalShotsFired) {
    s_telemetryLocalPawn = localPawn;
    s_prevLocalShotsFired = 0;
    s_seenImpactKeys.clear();
    s_seenImpactHistory.clear();
    s_pendingShots.clear();
    s_lastEnemyHealth.clear();
    s_recentShotEvents.clear();
    s_recentTraceEvents.clear();
    s_recentHitEvents.clear();
  }

  const float now = frameTimeSeconds;
  PruneSeenImpacts(now);

  const int newShots = std::max(0, localShotsFired - s_prevLocalShotsFired);
  if (newShots > 0) {
    const SDK::Vector2 shootAngles =
        ChooseTracerAngles(localAngles, localShootAngle);

    const SDK::Vector3 forward = ForwardFromAngles(shootAngles);
    const SDK::Vector3 start = localEyePos;
    const float weaponRange = std::clamp(
        localWeaponRange > 1.0f ? localWeaponRange : kTraceFallbackRange,
        kTraceMinRange, kTraceMaxRange);
    const SDK::Vector3 predictedEnd = {start.x + forward.x * weaponRange,
                                       start.y + forward.y * weaponRange,
                                       start.z + forward.z * weaponRange};

    for (int i = 0; i < newShots; ++i) {
      SDK::ShotEvent shot{};
      shot.id = NextTelemetryId();
      shot.shotsFired = localShotsFired - newShots + i + 1;
      shot.start = start;
      shot.angle = shootAngles;
      shot.predictedEnd = predictedEnd;
      shot.timeSeconds = now;
      s_recentShotEvents.push_back(shot);
      s_pendingShots.push_back(
          {shot.id, shot.shotsFired, shot.start, shot.predictedEnd, shot.angle,
           now, false, false});
    }
  }
  s_prevLocalShotsFired = localShotsFired;

  for (const auto &impact : localBulletImpacts) {
    if (impact.position.x == 0.0f && impact.position.y == 0.0f &&
        impact.position.z == 0.0f) {
      continue;
    }

    const uint64_t impactKey = MakeImpactKey(impact);
    if (!s_seenImpactKeys.insert(impactKey).second) {
      continue;
    }
    s_seenImpactHistory.push_back({impactKey, now});

    PendingShot *matchedPending = FindBestPendingShotForImpact(impact.position, now);
    if (matchedPending != nullptr) {
      matchedPending->impactMatched = true;
    }

    SDK::BulletTraceEvent trace{};
    trace.id = NextTelemetryId();
    trace.shotId = matchedPending != nullptr ? matchedPending->shotId : 0;
    trace.start = matchedPending != nullptr ? matchedPending->start : localEyePos;
    trace.end = impact.position;
    trace.confirmedImpact = true;
    trace.timeSeconds = now;
    s_recentTraceEvents.push_back(trace);
  }

  for (auto &pending : s_pendingShots) {
    const float age = now - pending.timeSeconds;
    if (!pending.fallbackEmitted && age >= kFallbackTraceDelaySeconds) {
      SDK::BulletTraceEvent trace{};
      trace.id = NextTelemetryId();
      trace.shotId = pending.shotId;
      trace.start = pending.start;
      trace.end = pending.predictedEnd;
      trace.confirmedImpact = false;
      trace.timeSeconds = now;
      s_recentTraceEvents.push_back(trace);
      pending.fallbackEmitted = true;
    }
  }

  s_pendingShots.erase(
      std::remove_if(s_pendingShots.begin(), s_pendingShots.end(),
                     [now](const PendingShot &pending) {
                       const float age = now - pending.timeSeconds;
                       if (pending.impactMatched) {
                         return age > kPendingImpactMatchWindowSeconds;
                       }
                       return age > kPendingShotExpireSeconds;
                     }),
      s_pendingShots.end());

  for (const auto &player : players) {
    if (!player.IsValid() || player.isTeammate || !player.IsAlive()) {
      continue;
    }

    const auto lastHealthIt = s_lastEnemyHealth.find(player.address);
    if (lastHealthIt != s_lastEnemyHealth.end() && player.health < lastHealthIt->second) {
      SDK::HitEvent hit{};
      hit.id = NextTelemetryId();
      hit.targetAddress = player.address;
      hit.position = BuildApproxHitPosition(player);
      hit.oldHealth = lastHealthIt->second;
      hit.newHealth = player.health;
      hit.timeSeconds = now;

      float bestTraceDistance = std::numeric_limits<float>::max();
      float bestShotDistance = std::numeric_limits<float>::max();
      uint64_t bestTraceShotId = 0;
      SDK::Vector3 bestTracePosition = hit.position;
      for (auto it = s_recentTraceEvents.rbegin(); it != s_recentTraceEvents.rend(); ++it) {
        const float dt = now - it->timeSeconds;
        if (dt < 0.0f || dt > kHitEventWindowSeconds) {
          continue;
        }

        const float distanceSq = DistanceSquared(it->end, hit.position);
        if (distanceSq < bestTraceDistance) {
          bestTraceDistance = distanceSq;
          bestTraceShotId = it->shotId;
          bestTracePosition = it->end;
          if (distanceSq <= kImpactMatchDistanceSq) {
            break;
          }
        }
      }

      if (bestTraceDistance <= kImpactLooseMatchDistanceSq) {
        hit.shotId = bestTraceShotId;
        hit.position = bestTracePosition;
      }

      if (hit.shotId == 0) {
        for (auto it = s_recentShotEvents.rbegin(); it != s_recentShotEvents.rend(); ++it) {
          const float dt = now - it->timeSeconds;
          if (dt < 0.0f || dt > kHitEventWindowSeconds) {
            continue;
          }

          const float distanceSq = PointToSegmentDistanceSquared(
              hit.position, it->start, it->predictedEnd);
          if (distanceSq < bestShotDistance) {
            bestShotDistance = distanceSq;
            hit.shotId = it->id;
          }
        }
      }

      if (hit.shotId != 0 || bestTraceDistance <= kImpactLooseMatchDistanceSq ||
          bestShotDistance <= kImpactLooseMatchDistanceSq) {
        s_recentHitEvents.push_back(hit);
      }
    }

    s_lastEnemyHealth[player.address] = player.health;
  }

  std::unordered_set<uintptr_t> activeEnemyAddresses;
  activeEnemyAddresses.reserve(players.size());
  std::unordered_set<uint32_t> activePawnHandles;
  activePawnHandles.reserve(players.size());

  for (const auto &player : players) {
    if (!player.IsValid() || !player.IsAlive()) {
      continue;
    }

    activePawnHandles.insert(player.pawnHandle);
    if (!player.isTeammate) {
      activeEnemyAddresses.insert(player.address);
    }

    MovementEventState &state = s_movementStates[player.pawnHandle];
    const bool wasInitialized = state.initialized;
    const float horizontalSpeed = std::sqrt(player.velocity.x * player.velocity.x +
                                            player.velocity.y * player.velocity.y);
    const float movedUnits = HorizontalDistance(player.renderPosition, state.position);
    const bool teleported = movedUnits >= kTeleportResetDistance;

    if (state.initialized) {
      const float strength = std::clamp(horizontalSpeed / 280.0f, 0.35f, 1.0f);

      auto pushAudioEvent = [&](SDK::MovementAudioType type, float eventStrength) {
        SDK::MovementAudioEvent event{};
        event.id = NextTelemetryId();
        event.pawnHandle = player.pawnHandle;
        event.playerAddress = player.address;
        event.origin = player.renderPosition;
        event.isTeammate = player.isTeammate;
        event.type = type;
        event.strength = std::clamp(eventStrength, 0.25f, 1.35f);
        event.timeSeconds = now;
        s_recentAudioEvents.push_back(event);
      };

      if (teleported) {
        state.accumulatedGroundDistance = 0.0f;
        state.lastStepPosition = player.renderPosition;
      }

      if (state.onGround && !player.isOnGround &&
          now - state.lastJumpTime >= 0.18f && horizontalSpeed >= 55.0f) {
        pushAudioEvent(SDK::MovementAudioType::Jump, std::max(0.45f, strength));
        state.lastJumpTime = now;
        state.accumulatedGroundDistance = 0.0f;
        state.airborneStartZ = state.position.z;
      }

      if (!state.onGround && player.isOnGround &&
          now - state.lastLandTime >= 0.18f) {
        const float verticalDelta =
            std::fabs(player.renderPosition.z - state.airborneStartZ);
        pushAudioEvent(SDK::MovementAudioType::Land,
                       std::clamp(0.55f + verticalDelta / 32.0f, 0.55f, 1.35f));
        state.lastLandTime = now;
        state.accumulatedGroundDistance = 0.0f;
        state.lastStepPosition = player.renderPosition;
      }

      if (player.isOnGround && !teleported) {
        if (horizontalSpeed >= kFootstepSpeedThreshold && movedUnits > 0.05f) {
          state.accumulatedGroundDistance += movedUnits;
          const float stepDistance = ComputeStepDistance(horizontalSpeed);
          if (state.accumulatedGroundDistance >= stepDistance &&
              now - state.lastFootstepTime >= 0.10f) {
            const float stepStrength = std::clamp(
                0.35f + (horizontalSpeed / 260.0f) +
                    std::min(state.accumulatedGroundDistance / stepDistance, 1.0f) *
                        0.15f,
                0.35f, 1.30f);
            pushAudioEvent(SDK::MovementAudioType::Footstep, stepStrength);
            state.lastFootstepTime = now;
            state.accumulatedGroundDistance =
                std::fmod(state.accumulatedGroundDistance, stepDistance);
            state.lastStepPosition = player.renderPosition;
          }
        } else if (horizontalSpeed < 18.0f) {
          state.accumulatedGroundDistance = 0.0f;
          state.lastStepPosition = player.renderPosition;
        }
      } else {
        state.accumulatedGroundDistance = 0.0f;
      }
    }

    state.initialized = true;
    state.onGround = player.isOnGround;
    state.horizontalSpeed = horizontalSpeed;
    state.position = player.renderPosition;
    if (!wasInitialized) {
      state.lastStepPosition = player.renderPosition;
    }
  }

  for (auto it = s_lastEnemyHealth.begin(); it != s_lastEnemyHealth.end();) {
    if (activeEnemyAddresses.find(it->first) == activeEnemyAddresses.end()) {
      it = s_lastEnemyHealth.erase(it);
    } else {
      ++it;
    }
  }

  for (auto it = s_movementStates.begin(); it != s_movementStates.end();) {
    if (activePawnHandles.find(it->first) == activePawnHandles.end()) {
      it = s_movementStates.erase(it);
    } else {
      ++it;
    }
  }

  PruneRecentEvents(s_recentShotEvents, now - kShotRetentionSeconds);
  PruneRecentEvents(s_recentTraceEvents, now - kTraceRetentionSeconds);
  PruneRecentEvents(s_recentHitEvents, now - kHitRetentionSeconds);
  PruneRecentEvents(s_recentAudioEvents, now - kAudioRetentionSeconds);

  shotEvents = s_recentShotEvents;
  bulletTraceEvents = s_recentTraceEvents;
  hitEvents = s_recentHitEvents;
  movementAudioEvents = s_recentAudioEvents;
}

bool GameManager::BuildFrameContext(FrameContext &context,
                                    const SDK::OffsetSet &offsets) {
  entityList = MemoryManager::Read<uintptr_t>(clientBase + offsets.dwEntityList);
  if (!entityList) {
    return false;
  }

  context.entityListObj = SDK::CEntityList(entityList, &offsets);
  context.offsets = &offsets;

  static uintptr_t s_lastFirstChunk = 0;
  const uintptr_t currentFirstChunk = context.entityListObj.GetListEntry(0);
  if (currentFirstChunk != s_lastFirstChunk) {
    s_lastFirstChunk = currentFirstChunk;
    InvalidateCachedEntityData();
  }

  context.localPlayer = SDK::CPlayerPawn(
      MemoryManager::Read<uintptr_t>(clientBase + offsets.dwLocalPlayerPawn), &offsets);

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
    SDK::CPlayerController controller(controllerPtr, context.offsets);
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

void GameManager::RebuildPlayers(const FrameContext &context, int localSlot,
                                 const SDK::OffsetSet &offsets) {
  constexpr float maxDistSq =
      Constants::ESP_MAX_DISTANCE_UNITS * Constants::ESP_MAX_DISTANCE_UNITS;

  for (uintptr_t controllerPtr : context.controllerPointers) {
    SDK::CPlayerController controller(controllerPtr, &offsets);
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

    const float interpFactor = s_interpolationFactor.load(std::memory_order_relaxed);
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

    entity.flags = MemoryManager::Read<uint32_t>(entity.address + offsets.m_fFlags);
    entity.velocity = MemoryManager::Read<SDK::Vector3>(entity.address + offsets.m_vecVelocity);
    entity.isOnGround = (entity.flags & 1) != 0;
    const SDK::Vector3 &vel = entity.velocity;
    entity.speed = std::sqrtf(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);

    if (!tooFar && s_readWeapons.load(std::memory_order_relaxed)) {
      entity.weapon = pawn.GetWeaponName();
    }

    if (!tooFar && s_readBones.load(std::memory_order_relaxed)) {
      const uintptr_t gameScene = pawn.GetGameSceneNode();
      if (gameScene > Constants::MIN_VALID_ADDRESS) {
        const uintptr_t boneArray = MemoryManager::Read<uintptr_t>(
            gameScene + offsets.m_boneArrayOffset);
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
  auto snapshot = std::make_shared<GameSnapshot>();
  snapshot->clientBase = clientBase;
  snapshot->entityList = entityList;
  snapshot->viewMatrix = viewMatrix;
  snapshot->players = players;
  snapshot->localPos = localPos;
  snapshot->localEyePos = localEyePos;
  snapshot->localAngles = localAngles;
  snapshot->localShootAngle = localShootAngle;
  snapshot->localAimPunch = localAimPunch;
  snapshot->localShotsFired = localShotsFired;
  snapshot->localTeam = localTeam;
  snapshot->localScoped = localScoped;
  snapshot->localCrosshairHandle = localCrosshairHandle;
  snapshot->localPawn = localPawn;
  snapshot->bombInfo = bombInfo;
  snapshot->localWeaponName = localWeaponName;
  snapshot->localWeaponRange = localWeaponRange;
  snapshot->localBulletImpacts = localBulletImpacts;
  snapshot->frameTimeSeconds = frameTimeSeconds;
  snapshot->shotEvents = shotEvents;
  snapshot->bulletTraceEvents = bulletTraceEvents;
  snapshot->hitEvents = hitEvents;
  snapshot->movementAudioEvents = movementAudioEvents;
  s_snapshot.store(std::static_pointer_cast<const GameSnapshot>(snapshot),
                   std::memory_order_release);
}

void GameManager::EnableBoneRead(bool enable) {
  s_readBones.store(enable, std::memory_order_relaxed);
}

void GameManager::EnableWeaponRead(bool enable) {
  s_readWeapons.store(enable, std::memory_order_relaxed);
}

void GameManager::SetInterpolationFactor(float factor) {
  s_interpolationFactor.store(factor, std::memory_order_relaxed);
}

void GameManager::SetScreenSize(int width, int height) {
  s_screenWidth.store(width, std::memory_order_relaxed);
  s_screenHeight.store(height, std::memory_order_relaxed);
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
  const int width = s_screenWidth.load(std::memory_order_relaxed);
  const int height = s_screenHeight.load(std::memory_order_relaxed);
  return (ndcX >= -1.0f - FRUSTUM_MARGIN / width &&
          ndcX <= 1.0f + FRUSTUM_MARGIN / width &&
          ndcY >= -1.0f - FRUSTUM_MARGIN / height &&
          ndcY <= 1.0f + FRUSTUM_MARGIN / height);
}

} // namespace Core
