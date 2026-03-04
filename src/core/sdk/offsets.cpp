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

// Aimbot / Triggerbot
ptrdiff_t m_angEyeAngles = 0x1384; // C_CSPlayerPawnBase::m_angEyeAngles
ptrdiff_t m_iCrosshairEntityHandle =
    0x13D8;                       // m_pObserverServices → crosshair entity
ptrdiff_t m_bIsScoped = 0x1404;   // C_CSPlayerPawn::m_bIsScoped
ptrdiff_t m_iShotsFired = 0x1368; // C_CSPlayerPawn::m_iShotsFired

} // namespace Offsets
} // namespace SDK
