#pragma once
#include "structs.h"
#include <cstdint>
#include <string>
#include <vector>

// Bone data layout: Vec3 position + 0x14 bytes padding = 0x1C per bone
struct BoneData {
  SDK::Vector3 pos;
  uint8_t pad[0x14];
};

// Bone indices (from reference)
enum BoneIndex : int {
  BONE_PELVIS = 0,
  BONE_SPINE_2 = 2,
  BONE_SPINE_1 = 4,
  BONE_NECK = 5,
  BONE_HEAD = 6,
  BONE_ARM_UP_L = 8,
  BONE_ARM_LO_L = 9,
  BONE_HAND_L = 10,
  BONE_ARM_UP_R = 13,
  BONE_ARM_LO_R = 14,
  BONE_HAND_R = 15,
  BONE_LEG_UP_L = 22,
  BONE_LEG_LO_L = 23,
  BONE_ANKLE_L = 24,
  BONE_LEG_UP_R = 25,
  BONE_LEG_LO_R = 26,
  BONE_ANKLE_R = 27,
  BONE_COUNT = 30,
};

// 16 bone connections to draw skeleton lines (including head)
inline constexpr int s_boneConnections[16][2] = {
    {BONE_HEAD, BONE_NECK}, // head
    {BONE_NECK, BONE_SPINE_1},      {BONE_SPINE_1, BONE_SPINE_2},
    {BONE_SPINE_2, BONE_PELVIS},    {BONE_NECK, BONE_ARM_UP_L},
    {BONE_ARM_UP_L, BONE_ARM_LO_L}, {BONE_ARM_LO_L, BONE_HAND_L},
    {BONE_NECK, BONE_ARM_UP_R},     {BONE_ARM_UP_R, BONE_ARM_LO_R},
    {BONE_ARM_LO_R, BONE_HAND_R},   {BONE_PELVIS, BONE_LEG_UP_L},
    {BONE_LEG_UP_L, BONE_LEG_LO_L}, {BONE_LEG_LO_L, BONE_ANKLE_L},
    {BONE_PELVIS, BONE_LEG_UP_R},   {BONE_LEG_UP_R, BONE_LEG_LO_R},
    {BONE_LEG_LO_R, BONE_ANKLE_R},
};

namespace SDK {
struct Entity {
  uintptr_t address = 0;
  uintptr_t controllerAddress = 0;
  int health = 0;
  int team = 0;
  bool isTeammate = false;
  float distance = 0.0f;
  Vector3 position = {0, 0, 0};
  std::string name;
  std::string weapon;
  uint32_t pawnHandle = 0; // Cached pawn handle for triggerbot/feature lookups
  // Bone positions (world space), indexed by BoneIndex enum
  std::vector<Vector3> bonePositions; // size 30 when valid

  bool isSpotted =
      false; // true = local player has line-of-sight (SpottedByMask)

  bool IsValid() const { return address != 0; }
  bool IsAlive() const { return health > 0 && health <= 100; }
};

struct BombInfo {
  bool isPlanted = false;
  float timeLeft = 0.0f;
  int site = -1; // 0 = A, 1 = B, -1 = None
};
} // namespace SDK
