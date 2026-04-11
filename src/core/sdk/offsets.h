#pragma once
#include <atomic>
#include <cstdint>
#include <memory>

namespace SDK {

struct OffsetSet {
  // -- Engine-level pointers (offsets.json) --
  ptrdiff_t dwEntityList = 0;
  ptrdiff_t dwLocalPlayerPawn = 0;
  ptrdiff_t dwLocalPlayerController = 0;
  ptrdiff_t dwViewMatrix = 0;
  ptrdiff_t dwPlantedC4 = 0;
  ptrdiff_t dwGlobalVars = 0;

  // -- Member offsets (client_dll.json) --
  ptrdiff_t m_fFlags = 0;
  ptrdiff_t m_vecVelocity = 0;
  ptrdiff_t m_iHealth = 0;
  ptrdiff_t m_iTeamNum = 0;
  ptrdiff_t m_vOldOrigin = 0;
  ptrdiff_t m_pGameSceneNode = 0;
  ptrdiff_t m_modelState = 0;
  ptrdiff_t m_hPlayerPawn = 0;
  ptrdiff_t m_iszPlayerName = 0;
  ptrdiff_t m_pClippingWeapon = 0;
  ptrdiff_t m_vecViewOffset = 0;
  ptrdiff_t m_flSimulationTime = 0;

  // -- Aimbot / Triggerbot --
  ptrdiff_t m_angEyeAngles = 0;
  ptrdiff_t m_aimPunchAngle = 0;
  ptrdiff_t m_iIDEntIndex = 0;
  ptrdiff_t m_bIsScoped = 0;
  ptrdiff_t m_iShotsFired = 0;

  // -- Bomb --
  ptrdiff_t m_nBombSite = 0;
  ptrdiff_t m_bBombTicking = 0;
  ptrdiff_t m_flTimerLength = 0;
  ptrdiff_t m_flC4Blow = 0;
  ptrdiff_t m_bBeingDefused = 0;
  ptrdiff_t m_flDefuseCountDown = 0;

  // -- Entity system internals (not exported by cs2-dumper) --
  ptrdiff_t m_hPawn = 0x6C4;
  ptrdiff_t m_bIsLocalPlayerController = 0x788;
  ptrdiff_t m_entitySpottedState = 0x26E0;
  ptrdiff_t m_bSpottedByMaskOffset = 0x00C;
  ptrdiff_t m_boneArrayOffset = 0x1E0;

  int MissingCount() const;
  bool HasRequired() const;
};

namespace Offsets {

std::shared_ptr<const OffsetSet> GetSnapshot();
OffsetSet GetCopy();
void Publish(const OffsetSet &offsets);
void Reset();

} // namespace Offsets
} // namespace SDK
