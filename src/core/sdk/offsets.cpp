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
  return dwEntityList > 0 && dwLocalPlayerPawn > 0 && dwViewMatrix > 0 &&
         m_fFlags > 0 && m_vecVelocity > 0 && m_iHealth > 0 &&
         m_iTeamNum > 0 && m_vOldOrigin > 0 && m_pGameSceneNode > 0 &&
         m_hPlayerPawn > 0 && m_iszPlayerName > 0 && m_vecViewOffset > 0 &&
         m_flSimulationTime > 0 && m_bIsLocalPlayerController > 0 &&
         m_entitySpottedState > 0 && m_bSpottedByMask > 0;
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
