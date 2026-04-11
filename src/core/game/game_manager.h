#pragma once
#include "core/constants.h"
#include "../sdk/entity.h"
#include "../sdk/entity_classes.h"
#include "../sdk/offsets.h"
#include "../sdk/structs.h"
#include <atomic>
#include <memory>
#include <unordered_map>
#include <vector>

namespace Core {

struct GameSnapshot {
  uintptr_t clientBase = 0;
  uintptr_t entityList = 0;
  SDK::Matrix4x4 viewMatrix = {};
  std::vector<SDK::Entity> players;
  SDK::Vector3 localPos = {};
  SDK::Vector3 localEyePos = {};
  SDK::Vector2 localAngles = {};
  SDK::Vector2 localAimPunch = {};
  int localShotsFired = 0;
  int localTeam = 0;
  bool localScoped = false;
  uint32_t localCrosshairHandle = 0;
  uintptr_t localPawn = 0;
  SDK::BombInfo bombInfo = {};
  std::string localWeaponName;
};

class GameManager {
public:
  static bool Init();
  static void Update();

  static void EnableBoneRead(bool enable);
  static void EnableWeaponRead(bool enable);
  static void SetInterpolationFactor(float factor);
  static void SetScreenSize(int width, int height);
  static bool IsOnScreen(const SDK::Vector3& worldPos);

  static std::shared_ptr<const GameSnapshot> GetSnapshot();
  static SDK::Matrix4x4 GetViewMatrix();
  static uintptr_t GetClientBase();
  static std::vector<SDK::Entity> GetRenderPlayers();
  static SDK::Vector3 GetLocalPos();
  static SDK::Vector3 GetLocalEyePos();
  static SDK::Vector2 GetLocalAngles();
  static SDK::Vector2 GetLocalAimPunch();
  static int GetLocalShotsFired();
  static int GetLocalTeam();
  static bool IsLocalScoped();
  static uint32_t GetLocalCrosshairEntityHandle();
  static uintptr_t GetLocalPlayerPawn();
  static uintptr_t GetEntityList();
  static uintptr_t GetEntityFromHandle(uint32_t handle);
  static std::string GetLocalWeaponName();
  static uintptr_t GetEntityGameSceneNode(uintptr_t entity);
  static SDK::BombInfo GetBombInfo();

private:
  struct FrameContext {
    SDK::CEntityList entityListObj;
    SDK::CPlayerPawn localPlayer;
    const SDK::OffsetSet* offsets = nullptr;
    uintptr_t controllerPointers[Constants::MAX_PLAYERS] = {};
  };

  static std::atomic<std::shared_ptr<const GameSnapshot>> s_snapshot;
  static uintptr_t clientBase;
  static std::atomic<bool> s_readBones;
  static std::atomic<bool> s_readWeapons;
  static std::atomic<float> s_interpolationFactor;
  static std::atomic<int> s_screenWidth;
  static std::atomic<int> s_screenHeight;
  static constexpr float FRUSTUM_MARGIN = 50.0f;

  static std::unordered_map<int, int> s_invalidSlotCache;
  static constexpr int INVALID_SLOT_SKIP_FRAMES = Constants::INVALID_SLOT_SKIP_FRAMES;

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
  static std::string localWeaponName;

  static void ResetLocalState();
  static void UpdateLocalState(const SDK::CPlayerPawn& localPlayer,
                               const SDK::OffsetSet& offsets);
  static void UpdateBombState(const SDK::CPlayerPawn& localPlayer,
                              const SDK::OffsetSet& offsets);
  static bool BuildFrameContext(FrameContext& context, const SDK::OffsetSet& offsets);
  static int FindLocalSlot(const FrameContext& context);
  static void DecrementInvalidSlotCache();
  static void RebuildPlayers(const FrameContext& context, int localSlot,
                             const SDK::OffsetSet& offsets);
  static void PublishFrameState();
  static void ClearFrameState(bool clearClientBase);
};

} // namespace Core
