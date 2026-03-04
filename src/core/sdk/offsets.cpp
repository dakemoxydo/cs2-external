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
ptrdiff_t m_angEyeAngles = 0x3DD0; // QAngle
ptrdiff_t m_iCrosshairEntityHandle = 0x13D8;
ptrdiff_t m_bIsScoped = 0x1404;
ptrdiff_t m_iShotsFired = 0x270C; // int32

} // namespace Offsets
} // namespace SDK
