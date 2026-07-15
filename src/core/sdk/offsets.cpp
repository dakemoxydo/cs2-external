#include "offsets.h"

namespace SDK {

namespace {
std::atomic<std::shared_ptr<const OffsetSet>> g_offsets{
    std::make_shared<const OffsetSet>()};
}

int OffsetSet::MissingCount() const {
  int count = 0;
#define CHECK(field) \
  if ((field) == 0)  \
  ++count
  CHECK(dwEntityList);
  CHECK(dwLocalPlayerPawn);
  CHECK(dwLocalPlayerController);
  CHECK(dwViewMatrix);
  CHECK(dwPlantedC4);
  CHECK(dwGlobalVars);
  CHECK(m_fFlags);
  CHECK(m_vecVelocity);
  CHECK(m_iHealth);
  CHECK(m_iTeamNum);
  CHECK(m_vOldOrigin);
  CHECK(m_pGameSceneNode);
  CHECK(m_modelState);
  CHECK(m_hPlayerPawn);
  CHECK(m_iszPlayerName);
  CHECK(m_pClippingWeapon);
  CHECK(m_vecViewOffset);
  CHECK(m_flSimulationTime);
  CHECK(m_bIsLocalPlayerController);
  CHECK(m_entitySpottedState);
  CHECK(m_bSpottedByMask);
  CHECK(m_angEyeAngles);
  CHECK(m_aimPunchAngle);
  CHECK(m_angShootAngleHistory);
  CHECK(m_iIDEntIndex);
  CHECK(m_bIsScoped);
  CHECK(m_iShotsFired);
  CHECK(m_flRange);
  CHECK(m_pBulletServices);
  CHECK(m_nBombSite);
  CHECK(m_bBombTicking);
  CHECK(m_flTimerLength);
  CHECK(m_flC4Blow);
  CHECK(m_bBeingDefused);
  CHECK(m_flDefuseCountDown);
#undef CHECK
  return count;
}

bool OffsetSet::HasRequired() const {
  // These are the only offsets needed to establish a usable game snapshot.
  // Feature-specific fields are allowed to be absent: cs2-dumper occasionally
  // renames or temporarily omits one of them, which must not invalidate the
  // complete offset generation and leave the application on a stale cache.
  return dwEntityList > 0 && dwLocalPlayerPawn > 0 && dwViewMatrix > 0;
}

namespace Offsets {

std::shared_ptr<const OffsetSet> GetSnapshot() {
  return std::atomic_load(&g_offsets);
}

OffsetSet GetCopy() {
  return *GetSnapshot();
}

void Publish(const OffsetSet &offsets) {
  std::atomic_store(&g_offsets, std::make_shared<const OffsetSet>(offsets));
}

void Reset() {
  Publish(OffsetSet{});
}

} // namespace Offsets

} // namespace SDK
