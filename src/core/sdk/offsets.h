// offsets.h — добавлены новые поля для aimbot/triggerbot
#pragma once
#include <cstdint>

namespace SDK {
namespace Offsets {
extern ptrdiff_t dwEntityList;
extern ptrdiff_t dwLocalPlayerPawn;
extern ptrdiff_t dwViewMatrix;
extern ptrdiff_t dwPlantedC4;

extern ptrdiff_t m_iHealth;
extern ptrdiff_t m_iTeamNum;
extern ptrdiff_t m_vOldOrigin;
extern ptrdiff_t m_pGameSceneNode;
extern ptrdiff_t m_modelState;
extern ptrdiff_t m_hPlayerPawn;
extern ptrdiff_t m_iszPlayerName;

// Aimbot / Triggerbot offsets
extern ptrdiff_t m_angEyeAngles; // view angles of localPawn (pitch, yaw)
extern ptrdiff_t m_iCrosshairEntityHandle; // entity handle crosshair is on
extern ptrdiff_t m_bIsScoped;              // AWP scope state
extern ptrdiff_t m_iShotsFired;            // shots fired count

} // namespace Offsets
} // namespace SDK
