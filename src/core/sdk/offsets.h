// offsets.h — все оффсеты проекта (включая внутренние поля entity-системы)
#pragma once
#include <cstdint>

namespace SDK {
namespace Offsets {
// ── Engine-level pointers (offsets.json) ──────────────────────────────────
extern ptrdiff_t dwEntityList;
extern ptrdiff_t dwLocalPlayerPawn;
extern ptrdiff_t dwViewMatrix;
extern ptrdiff_t dwPlantedC4;

// ── Member offsets (client_dll.json) ──────────────────────────────────────
extern ptrdiff_t m_iHealth;
extern ptrdiff_t m_iTeamNum;
extern ptrdiff_t m_vOldOrigin;
extern ptrdiff_t m_pGameSceneNode;
extern ptrdiff_t m_modelState;
extern ptrdiff_t m_hPlayerPawn;
extern ptrdiff_t m_iszPlayerName;
extern ptrdiff_t m_pClippingWeapon;
extern ptrdiff_t m_vecViewOffset;

// ── Aimbot / Triggerbot
extern ptrdiff_t m_angEyeAngles;           // src: cs2-dumper, client_dll
extern ptrdiff_t m_aimPunchAngle;
extern ptrdiff_t m_iCrosshairEntityHandle; // src: cs2-dumper, client_dll
extern ptrdiff_t m_bIsScoped;              // src: cs2-dumper, client_dll
extern ptrdiff_t m_iShotsFired;            // src: cs2-dumper, client_dll

// ── Bomb ──────────────────────────────────────────────────────────────────
extern ptrdiff_t m_nBombSite;     // src: cs2-dumper, client_dll
extern ptrdiff_t m_bBombTicking;  // src: cs2-dumper, client_dll
extern ptrdiff_t m_flTimerLength; // src: cs2-dumper, client_dll

// ── Entity system internals (не экспортируются cs2-dumper напрямую) ───────
extern ptrdiff_t m_hPawn;                    // CPlayerController::m_hPawn
extern ptrdiff_t m_bIsLocalPlayerController; // CPlayerController, bool
extern ptrdiff_t m_entitySpottedState;   // C_CSPlayerPawn spotted state base
extern ptrdiff_t m_bSpottedByMaskOffset; // +0xC от m_entitySpottedState
extern ptrdiff_t m_boneArrayOffset;      // CGameSceneNode → bone array ptr

} // namespace Offsets
} // namespace SDK
