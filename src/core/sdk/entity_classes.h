#pragma once
#include <cstdint>
#include <string>
#include "../memory/memory_manager.h"
#include "offsets.h"
#include "structs.h"

namespace SDK {

class CEntityInstance {
protected:
  uintptr_t address;
public:
  CEntityInstance(uintptr_t addr = 0) : address(addr) {}
  bool IsValid() const {
    return address > 0x10000 && address < 0x7FFFFFFFFFFFFFFF;
  }
  uintptr_t GetAddress() const { return address; }
};

class CPlayerPawn : public CEntityInstance {
public:
  using CEntityInstance::CEntityInstance;

  int GetHealth() const {
    return Core::MemoryManager::Read<int>(address + Offsets::m_iHealth);
  }
  
  int GetTeam() const {
    return Core::MemoryManager::Read<int>(address + Offsets::m_iTeamNum);
  }
  
  Vector3 GetOldOrigin() const {
    return Core::MemoryManager::Read<Vector3>(address + Offsets::m_vOldOrigin);
  }
  
  Vector3 GetCameraPos() const {
    Vector3 origin = GetOldOrigin();
    Vector3 viewOffset = Core::MemoryManager::Read<Vector3>(address + Offsets::m_vecViewOffset);
    return { origin.x + viewOffset.x, origin.y + viewOffset.y, origin.z + viewOffset.z };
  }
  
  Vector2 GetEyeAngles() const {
    return Core::MemoryManager::Read<Vector2>(address + Offsets::m_angEyeAngles);
  }

  bool IsScoped() const {
    return Core::MemoryManager::Read<bool>(address + Offsets::m_bIsScoped);
  }

  Vector2 GetAimPunch() const {
    return Core::MemoryManager::Read<Vector2>(address + Offsets::m_aimPunchAngle);
  }

  uint32_t GetCrosshairEntityHandle() const {
    return Core::MemoryManager::Read<uint32_t>(address + Offsets::m_iCrosshairEntityHandle);
  }

  int GetShotsFired() const {
    return Core::MemoryManager::Read<int>(address + Offsets::m_iShotsFired);
  }

  uint32_t GetSpottedStateMask() const {
    return Core::MemoryManager::Read<uint32_t>(address + Offsets::m_entitySpottedState +
                                               Offsets::m_bSpottedByMaskOffset);
  }

  uintptr_t GetGameSceneNode() const {
    return Core::MemoryManager::Read<uintptr_t>(address + Offsets::m_pGameSceneNode);
  }

  std::string GetWeaponName() const {
    uintptr_t nPtr = Core::MemoryManager::ReadChain<uintptr_t>(
        address, {(uintptr_t)Offsets::m_pClippingWeapon, 0x10, 0x20});
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
};

class CPlayerController : public CEntityInstance {
public:
  using CEntityInstance::CEntityInstance;

  uint32_t GetPawnHandle() const {
    return Core::MemoryManager::Read<uint32_t>(address + Offsets::m_hPawn);
  }
  
  bool IsLocalPlayerController() const {
    return Core::MemoryManager::Read<bool>(address + Offsets::m_bIsLocalPlayerController);
  }
  
  std::string GetPlayerName() const {
    char nameBuffer[128] = {};
    Core::MemoryManager::ReadRaw(address + Offsets::m_iszPlayerName, nameBuffer,
                                 sizeof(nameBuffer) - 1);
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
    if (!listEntry) return CPlayerController(0);
    uintptr_t ptr = Core::MemoryManager::Read<uintptr_t>(listEntry + (index + 1) * 0x70);
    return CPlayerController(ptr);
  }

  CPlayerPawn GetPawnFromHandle(uint32_t handle, uintptr_t cachedListEntry = 0) const {
    if (!handle || handle == 0xFFFFFFFF) return CPlayerPawn(0);
    
    int chunkIdx = (int)((handle & 0x7FFF) >> 9);
    uintptr_t entry = cachedListEntry ? cachedListEntry : GetListEntry(chunkIdx);
    if (!entry) return CPlayerPawn(0);

    uintptr_t pawnAddr = Core::MemoryManager::Read<uintptr_t>(entry + (handle & 0x1FF) * 0x70);
    return CPlayerPawn(pawnAddr);
  }
};

class CPlantedC4 : public CEntityInstance {
public:
  using CEntityInstance::CEntityInstance;

  bool IsTicking() const {
    return Core::MemoryManager::Read<bool>(address + Offsets::m_bBombTicking);
  }
  int GetSite() const {
    return Core::MemoryManager::Read<int>(address + Offsets::m_nBombSite);
  }
  float GetTimeLeft() const {
    return Core::MemoryManager::Read<float>(address + Offsets::m_flTimerLength);
  }
};

} // namespace SDK
