#pragma once
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "../memory/memory_manager.h"
#include "offsets.h"
#include "structs.h"

namespace SDK {

class CEntityInstance {
protected:
  uintptr_t address;
  const OffsetSet* offsets;

  const OffsetSet& GetOffsets() const {
    static const OffsetSet emptyOffsets{};
    return offsets ? *offsets : emptyOffsets;
  }
public:
  CEntityInstance(uintptr_t addr = 0, const OffsetSet* offsetSet = nullptr)
      : address(addr), offsets(offsetSet) {}
  bool IsValid() const {
    return address > 0x10000 && address < 0x7FFFFFFFFFFFFFFF;
  }
  uintptr_t GetAddress() const { return address; }
};

class CPlayerPawn : public CEntityInstance {
public:
  using CEntityInstance::CEntityInstance;

  int GetHealth() const {
    if (GetOffsets().m_iHealth == 0) return 0;
    return Core::MemoryManager::Read<int>(address + GetOffsets().m_iHealth);
  }
  
  int GetTeam() const {
    if (GetOffsets().m_iTeamNum == 0) return 0;
    return Core::MemoryManager::Read<int>(address + GetOffsets().m_iTeamNum);
  }
  
  Vector3 GetOldOrigin() const {
    if (GetOffsets().m_vOldOrigin == 0) return {};
    return Core::MemoryManager::Read<Vector3>(address + GetOffsets().m_vOldOrigin);
  }
  
  Vector3 GetCameraPos() const {
    Vector3 origin = GetOldOrigin();
    if (GetOffsets().m_vecViewOffset == 0) return origin;
    Vector3 viewOffset =
        Core::MemoryManager::Read<Vector3>(address + GetOffsets().m_vecViewOffset);
    return { origin.x + viewOffset.x, origin.y + viewOffset.y, origin.z + viewOffset.z };
  }
  
  Vector2 GetEyeAngles() const {
    if (GetOffsets().m_angEyeAngles == 0) return {};
    return Core::MemoryManager::Read<Vector2>(address + GetOffsets().m_angEyeAngles);
  }

  bool IsScoped() const {
    if (GetOffsets().m_bIsScoped == 0) return false;
    return Core::MemoryManager::Read<bool>(address + GetOffsets().m_bIsScoped);
  }

  Vector2 GetAimPunch() const {
    if (GetOffsets().m_aimPunchAngle == 0) return {};
    return Core::MemoryManager::Read<Vector2>(address + GetOffsets().m_aimPunchAngle);
  }

  uint32_t GetCrosshairEntityHandle() const {
    if (GetOffsets().m_iIDEntIndex == 0) return 0;
    return Core::MemoryManager::Read<uint32_t>(address + GetOffsets().m_iIDEntIndex);
  }

  int GetShotsFired() const {
    if (GetOffsets().m_iShotsFired == 0) return 0;
    return Core::MemoryManager::Read<int>(address + GetOffsets().m_iShotsFired);
  }

  Vector2 GetShootAngle() const {
    const auto &O = GetOffsets();
    if (O.m_angShootAngleHistory == 0) {
      return GetEyeAngles();
    }

    Vector2 history[2] = {};
    if (!Core::MemoryManager::ReadRaw(address + O.m_angShootAngleHistory, history,
                                      sizeof(history))) {
      return GetEyeAngles();
    }

    const auto valid = [](const Vector2 &angle) {
      return std::isfinite(angle.x) && std::isfinite(angle.y) &&
             (angle.x != 0.0f || angle.y != 0.0f);
    };

    if (valid(history[0])) {
      return history[0];
    }
    if (valid(history[1])) {
      return history[1];
    }
    return GetEyeAngles();
  }

  float GetSimulationTime() const {
    if (GetOffsets().m_flSimulationTime == 0) return 0.0f;
    return Core::MemoryManager::Read<float>(address + GetOffsets().m_flSimulationTime);
  }

  uint64_t GetSpottedStateMask() const {
    const auto& O = GetOffsets();
    if (O.m_entitySpottedState == 0 || O.m_bSpottedByMask == 0) return 0;
    return Core::MemoryManager::Read<uint64_t>(address + O.m_entitySpottedState +
                                               O.m_bSpottedByMask);
  }

  uintptr_t GetGameSceneNode() const {
    if (GetOffsets().m_pGameSceneNode == 0) return 0;
    return Core::MemoryManager::Read<uintptr_t>(address + GetOffsets().m_pGameSceneNode);
  }

  std::string GetWeaponName() const {
    const auto& O = GetOffsets();
    if (O.m_pClippingWeapon == 0) return {};
    uintptr_t nPtr = Core::MemoryManager::ReadChain<uintptr_t>(
        address, {(uintptr_t)O.m_pClippingWeapon, 0x10, 0x20});
    if (nPtr > 0x10000) {
      char wb[64] = {};
      Core::MemoryManager::ReadRaw(nPtr, wb, sizeof(wb) - 1);
      std::string name(wb);
      if (name.rfind("weapon_", 0) == 0)
        return name.substr(7);
      return name;
    }
    return "";
  }

  float GetWeaponRange() const {
    const auto &O = GetOffsets();
    if (O.m_pClippingWeapon == 0 || O.m_flRange == 0) {
      return 0.0f;
    }

    const uintptr_t clippingWeapon =
        Core::MemoryManager::Read<uintptr_t>(address + O.m_pClippingWeapon);
    if (clippingWeapon <= 0x10000) {
      return 0.0f;
    }

    const uintptr_t weaponData =
        Core::MemoryManager::Read<uintptr_t>(clippingWeapon + 0x10);
    if (weaponData <= 0x10000) {
      return 0.0f;
    }

    return Core::MemoryManager::Read<float>(weaponData + O.m_flRange);
  }

  std::vector<BulletImpactInfo> GetBulletImpacts() const {
    std::vector<BulletImpactInfo> impacts;
    const auto &O = GetOffsets();
    if (O.m_pBulletServices == 0) {
      return impacts;
    }

    const uintptr_t bulletServices =
        Core::MemoryManager::Read<uintptr_t>(address + O.m_pBulletServices);
    if (bulletServices <= 0x10000) {
      return impacts;
    }

    constexpr uintptr_t kBulletVectorOffset = 0x48;
    constexpr int kMaxImpacts = 64;

    const uintptr_t vectorBase = bulletServices + kBulletVectorOffset;
    const uintptr_t dataPtr = Core::MemoryManager::Read<uintptr_t>(vectorBase);
    if (dataPtr <= 0x10000) {
      return impacts;
    }

    int size = Core::MemoryManager::Read<int>(vectorBase + 0x10);
    if (size <= 0 || size > kMaxImpacts) {
      size = Core::MemoryManager::Read<int>(vectorBase + 0x08);
    }
    if (size <= 0 || size > kMaxImpacts) {
      return impacts;
    }

    impacts.resize(size);
    if (!Core::MemoryManager::ReadRaw(dataPtr, impacts.data(),
                                      sizeof(BulletImpactInfo) * size)) {
      impacts.clear();
    }
    return impacts;
  }
};

class CPlayerController : public CEntityInstance {
public:
  using CEntityInstance::CEntityInstance;

  uint32_t GetPawnHandle() const {
    const auto &pawnOffsets = GetOffsets();
    if (pawnOffsets.m_hPlayerPawn == 0) return 0;
    return Core::MemoryManager::Read<uint32_t>(address + pawnOffsets.m_hPlayerPawn);
  }
  
  bool IsLocalPlayerController() const {
    if (GetOffsets().m_bIsLocalPlayerController == 0) return false;
    return Core::MemoryManager::Read<bool>(address + GetOffsets().m_bIsLocalPlayerController);
  }
  
  std::string GetPlayerName() const {
    if (GetOffsets().m_iszPlayerName == 0) return {};
    char nameBuffer[128] = {};
    Core::MemoryManager::ReadRaw(address + GetOffsets().m_iszPlayerName,
                                 nameBuffer, sizeof(nameBuffer) - 1);
    return std::string(nameBuffer);
  }
};

class CEntityList : public CEntityInstance {
public:
  using CEntityInstance::CEntityInstance;

  uintptr_t GetListEntry(int chunkIndex) const {
    return Core::MemoryManager::Read<uintptr_t>(address + 0x10 + 0x8 * chunkIndex);
  }

  CPlayerController GetController(int index) const {
    uintptr_t listEntry = GetListEntry(0);
    if (!listEntry) return CPlayerController(0, offsets);
    uintptr_t ptr = Core::MemoryManager::Read<uintptr_t>(listEntry + (index + 1) * 0x70);
    return CPlayerController(ptr, offsets);
  }

  CPlayerPawn GetPawnFromHandle(uint32_t handle, uintptr_t cachedListEntry = 0) const {
    if (!handle || handle == 0xFFFFFFFF) return CPlayerPawn(0, offsets);
    
    int chunkIdx = (int)((handle & 0x7FFF) >> 9);
    uintptr_t entry = cachedListEntry ? cachedListEntry : GetListEntry(chunkIdx);
    if (!entry) return CPlayerPawn(0, offsets);

    uintptr_t pawnAddr = Core::MemoryManager::Read<uintptr_t>(entry + (handle & 0x1FF) * 0x70);
    return CPlayerPawn(pawnAddr, offsets);
  }
};

class CPlantedC4 : public CEntityInstance {
public:
  using CEntityInstance::CEntityInstance;

  bool IsTicking() const {
    if (GetOffsets().m_bBombTicking == 0) return false;
    return Core::MemoryManager::Read<bool>(address + GetOffsets().m_bBombTicking);
  }
  int GetSite() const {
    if (GetOffsets().m_nBombSite == 0) return -1;
    return Core::MemoryManager::Read<int>(address + GetOffsets().m_nBombSite);
  }
  float GetTimerLength() const {
    if (GetOffsets().m_flTimerLength == 0) return 0.0f;
    return Core::MemoryManager::Read<float>(address + GetOffsets().m_flTimerLength);
  }
  float GetBlowTime() const {
    if (GetOffsets().m_flC4Blow == 0) return 0.0f;
    return Core::MemoryManager::Read<float>(address + GetOffsets().m_flC4Blow);
  }
  bool IsBeingDefused() const {
    if (GetOffsets().m_bBeingDefused == 0) return false;
    return Core::MemoryManager::Read<bool>(address + GetOffsets().m_bBeingDefused);
  }
  float GetDefuseCountDown() const {
    if (GetOffsets().m_flDefuseCountDown == 0) return 0.0f;
    return Core::MemoryManager::Read<float>(address + GetOffsets().m_flDefuseCountDown);
  }
};

} // namespace SDK
