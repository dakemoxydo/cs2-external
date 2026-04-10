#pragma once
#include "core/constants.h"
#include "../sdk/entity.h"
#include "../sdk/entity_classes.h"
#include "../sdk/structs.h"
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace Core {

// Thread-safety model:
//
// Double-buffering for entity list (lock-free read path):
//   - Memory Thread writes to playerBuffers[inactive], then atomically swaps activeBufferIndex
//   - Render Thread reads from playerBuffers[active] via GetRenderPlayers()
//   - bufferMutex protects the copy operation during swap (brief hold)
//   - activeBufferIndex is std::atomic with acquire/release semantics
//
// Shared mutex for scalar state:
//   - stateMutex protects all cached* variables (viewMatrix, localPos, etc.)
//   - Memory Thread takes unique_lock to write all cached values at once
//   - Render Thread getters take shared_lock for concurrent reads
//
// Atomic flags:
//   - s_readBones, s_readWeapons — relaxed atomics, written by render thread, read by memory thread

class GameManager {
public:
  static bool Init();
  static void Update();

  static void EnableBoneRead(bool enable);
  static void EnableWeaponRead(bool enable);
  static void SetInterpolationFactor(float factor); // 0.0-1.0, default 0.5
  static void SetScreenSize(int width, int height);
  static bool IsOnScreen(const SDK::Vector3& worldPos);

  // Rendering getters (thread-safe using shared_mutex)
  static SDK::Matrix4x4 GetViewMatrix();
  static uintptr_t GetClientBase();
  static std::vector<SDK::Entity> GetRenderPlayers();

  // Local player accessors (used by radar, features)
  static SDK::Vector3 GetLocalPos();
  static SDK::Vector3 GetLocalEyePos();
  static SDK::Vector2 GetLocalAngles(); // Use .y for yaw
  static SDK::Vector2 GetLocalAimPunch();
  static int GetLocalShotsFired();
  static int GetLocalTeam();
  static bool IsLocalScoped();
  static uint32_t GetLocalCrosshairEntityHandle();

  // Engine pointers for external feature logic
  static uintptr_t GetLocalPlayerPawn();
  static uintptr_t GetEntityList();
  // NOTE: Makes a live RPM call — call only from memory thread, not render thread
  static uintptr_t GetEntityFromHandle(uint32_t handle);

  // Weapon and entity data accessors (to avoid MemoryManager reads in features)
  static std::string GetLocalWeaponName();
  static uintptr_t GetEntityGameSceneNode(uintptr_t entity);

  // Bomb State
  static SDK::BombInfo GetBombInfo();

private:
  struct FrameContext {
    SDK::CEntityList entityListObj;
    SDK::CPlayerPawn localPlayer;
    uintptr_t controllerPointers[Constants::MAX_PLAYERS] = {};
  };

  static uintptr_t clientBase;
  static std::atomic<bool> s_readBones;
  static std::atomic<bool> s_readWeapons;
  static float s_interpolationFactor;
  static int s_screenWidth;
  static int s_screenHeight;
  static constexpr float FRUSTUM_MARGIN = 50.0f;

  // Cache for invalid entity slots (skip reading for N frames)
  static std::unordered_map<int, int> s_invalidSlotCache;
  static constexpr int INVALID_SLOT_SKIP_FRAMES = Constants::INVALID_SLOT_SKIP_FRAMES;

  // -- Backend State (Written by Memory Thread) --
  static SDK::Matrix4x4 viewMatrix;
  static std::vector<SDK::Entity> players;
  static SDK::Vector3 localPos;
  static SDK::Vector3 localEyePos;
  static SDK::Vector2 localAngles;
  static SDK::Vector2 localAimPunch;
  static int localShotsFired;
  static int localTeam;
  static bool localScoped;
  static uint32_t localCrosshairHandle;
  static uintptr_t localPawn;
  static uintptr_t entityList;
  static SDK::BombInfo bombInfo;

  // -- Double-buffered entity list (Memory Thread writes, Render Thread reads) --
  // Write to inactive buffer, atomic swap index, read from active buffer.
  // bufferMutex protects the vector copy during swap (brief hold).
  static std::vector<SDK::Entity> playerBuffers[2];
  static std::atomic<int> activeBufferIndex; // acquire/release for cross-thread visibility
  static std::mutex bufferMutex;

  // -- Cached scalar state (protected by stateMutex) --
  // Memory Thread: unique_lock to write all at end of Update()
  // Render Thread: shared_lock for concurrent reads via getters
  static std::shared_mutex stateMutex;
  static SDK::Matrix4x4 cachedViewMatrix;
  static SDK::Vector3 cachedLocalPos;
  static SDK::Vector3 cachedLocalEyePos;
  static SDK::Vector2 cachedLocalAngles;
  static SDK::Vector2 cachedLocalAimPunch;
  static int cachedLocalShotsFired;
  static int cachedLocalTeam;
  static bool cachedLocalScoped;
  static uint32_t cachedLocalCrosshairHandle;
  static uintptr_t cachedLocalPawn;
  static uintptr_t cachedEntityList;
  static SDK::BombInfo cachedBombInfo;

  static void ResetLocalState();
  static void UpdateLocalState(const SDK::CPlayerPawn& localPlayer);
  static void UpdateBombState();
  static bool BuildFrameContext(FrameContext& context);
  static int FindLocalSlot(const FrameContext& context);
  static void DecrementInvalidSlotCache();
  static void RebuildPlayers(const FrameContext& context, int localSlot);
  static void PublishFrameState();
};
} // namespace Core
