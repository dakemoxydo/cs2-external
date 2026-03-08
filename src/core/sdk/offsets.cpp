#include "offsets.h"

namespace SDK {
namespace Offsets {
ptrdiff_t dwEntityList = 0;
ptrdiff_t dwLocalPlayerPawn = 0;
ptrdiff_t dwViewMatrix = 0;
ptrdiff_t dwPlantedC4 = 0;

ptrdiff_t m_iHealth = 0;
ptrdiff_t m_iTeamNum = 0;
ptrdiff_t m_vOldOrigin = 0;
ptrdiff_t m_pGameSceneNode = 0;
ptrdiff_t m_modelState = 0;
ptrdiff_t m_hPlayerPawn = 0;
ptrdiff_t m_iszPlayerName = 0;
ptrdiff_t m_pClippingWeapon = 0x3DC0;

ptrdiff_t m_pWeaponServices = 0;
ptrdiff_t m_hActiveWeapon = 0;
ptrdiff_t m_AttributeManager = 0;
ptrdiff_t m_Item = 0;
ptrdiff_t m_iItemDefinitionIndex = 0;
ptrdiff_t m_hModel = 0;
ptrdiff_t m_iItemIDHigh = 0;
ptrdiff_t m_nFallbackPaintKit = 0;
ptrdiff_t m_flFallbackWear = 0;
ptrdiff_t m_nFallbackSeed = 0;

// Aimbot / Triggerbot
ptrdiff_t m_angEyeAngles = 0x3DD0;
ptrdiff_t m_iCrosshairEntityHandle = 0x13D8;
ptrdiff_t m_bIsScoped = 0x1404;
ptrdiff_t m_iShotsFired = 0x270C;

// Bomb
ptrdiff_t m_nBombSite = 0x119C;
ptrdiff_t m_bBombTicking = 0x120C;
ptrdiff_t m_flTimerLength = 0x1210;

// Entity system internals — не экспортируются cs2-dumper, хардкод из
// реверс-анализа
ptrdiff_t m_hPawn = 0x6C4; // CPlayerController::m_hPawn
ptrdiff_t m_bIsLocalPlayerController =
    0x788; // CPlayerController::m_bIsLocalPlayerController
ptrdiff_t m_entitySpottedState = 0x26E0; // C_CSPlayerPawn::m_entitySpottedState
ptrdiff_t m_bSpottedByMaskOffset =
    0x00C; // +0xC от m_entitySpottedState = m_bSpottedByMask[2]
ptrdiff_t m_boneArrayOffset =
    0x1E0; // CGameSceneNode → bone array ptr offset (0x160 + 0x80)

} // namespace Offsets
} // namespace SDK
