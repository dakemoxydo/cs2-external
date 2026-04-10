// offsets.h - all project offsets (including internal entity-system fields)
// Merged: declarations + definitions in single header using inline variables
#pragma once
#include <cstdint>

namespace SDK {
namespace Offsets {
// -- Engine-level pointers (offsets.json) --
inline ptrdiff_t dwEntityList = 0;
inline ptrdiff_t dwLocalPlayerPawn = 0;
inline ptrdiff_t dwLocalPlayerController = 0;
inline ptrdiff_t dwViewMatrix = 0;
inline ptrdiff_t dwPlantedC4 = 0;

// -- Member offsets (client_dll.json) --
inline ptrdiff_t m_fFlags = 0;
inline ptrdiff_t m_vecVelocity = 0;
inline ptrdiff_t m_iHealth = 0;
inline ptrdiff_t m_iTeamNum = 0;
inline ptrdiff_t m_vOldOrigin = 0;
inline ptrdiff_t m_pGameSceneNode = 0;
inline ptrdiff_t m_modelState = 0;
inline ptrdiff_t m_hPlayerPawn = 0;
inline ptrdiff_t m_iszPlayerName = 0;
inline ptrdiff_t m_pClippingWeapon = 0;
inline ptrdiff_t m_vecViewOffset = 0;

// -- Aimbot / Triggerbot --
inline ptrdiff_t m_angEyeAngles = 0;
inline ptrdiff_t m_aimPunchAngle = 0;
inline ptrdiff_t m_iIDEntIndex = 0;
inline ptrdiff_t m_bIsScoped = 0;
inline ptrdiff_t m_iShotsFired = 0;

// -- Bomb --
inline ptrdiff_t m_nBombSite = 0;
inline ptrdiff_t m_bBombTicking = 0;
inline ptrdiff_t m_flTimerLength = 0;

// -- Entity system internals (not exported by cs2-dumper) --
inline ptrdiff_t m_hPawn = 0x6C4;
inline ptrdiff_t m_bIsLocalPlayerController = 0x788;
inline ptrdiff_t m_entitySpottedState = 0x26E0;
inline ptrdiff_t m_bSpottedByMaskOffset = 0x00C;
inline ptrdiff_t m_boneArrayOffset = 0x1E0;

} // namespace Offsets
} // namespace SDK
