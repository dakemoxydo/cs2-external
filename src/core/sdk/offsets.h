// offsets.h — все оффсеты проекта (включая внутренние поля entity-системы)
// Merged: declarations + definitions in single header using inline variables
#pragma once
#include <cstdint>

namespace SDK {
namespace Offsets {
// ── Engine-level pointers (offsets.json) ──────────────────────────────────
inline ptrdiff_t dwEntityList = 0;
inline ptrdiff_t dwLocalPlayerPawn = 0;
inline ptrdiff_t dwViewMatrix = 0;
inline ptrdiff_t dwPlantedC4 = 0;

// ── Member offsets (client_dll.json) ──────────────────────────────────────
inline ptrdiff_t m_iHealth = 0;
inline ptrdiff_t m_iTeamNum = 0;
inline ptrdiff_t m_vOldOrigin = 0;
inline ptrdiff_t m_pGameSceneNode = 0;
inline ptrdiff_t m_modelState = 0;
inline ptrdiff_t m_hPlayerPawn = 0;
inline ptrdiff_t m_iszPlayerName = 0;
inline ptrdiff_t m_pClippingWeapon = 0x3DC0;
inline ptrdiff_t m_vecViewOffset = 0xC58;

// ── Aimbot / Triggerbot
inline ptrdiff_t m_angEyeAngles = 0x3DD0;           // src: cs2-dumper, client_dll
inline ptrdiff_t m_aimPunchAngle = 0x1774;
inline ptrdiff_t m_iIDEntIndex = 0x13D8;            // src: cs2-dumper, client_dll (C_CSPlayerPawn)
inline ptrdiff_t m_bIsScoped = 0x1404;              // src: cs2-dumper, client_dll
inline ptrdiff_t m_iShotsFired = 0x270C;            // src: cs2-dumper, client_dll

// ── Bomb ──────────────────────────────────────────────────────────────────
inline ptrdiff_t m_nBombSite = 0x119C;     // src: cs2-dumper, client_dll
inline ptrdiff_t m_bBombTicking = 0x120C;  // src: cs2-dumper, client_dll
inline ptrdiff_t m_flTimerLength = 0x1210; // src: cs2-dumper, client_dll

// ── Entity system internals (не экспортируются cs2-dumper напрямую) ───────
inline ptrdiff_t m_hPawn = 0x6C4;                    // CPlayerController::m_hPawn
inline ptrdiff_t m_bIsLocalPlayerController = 0x788; // CPlayerController::m_bIsLocalPlayerController
inline ptrdiff_t m_entitySpottedState = 0x26E0;      // C_CSPlayerPawn spotted state base
inline ptrdiff_t m_bSpottedByMaskOffset = 0x00C;     // +0xC от m_entitySpottedState
inline ptrdiff_t m_boneArrayOffset = 0x1E0;          // CGameSceneNode → bone array ptr offset (0x160 + 0x80)

} // namespace Offsets
} // namespace SDK
