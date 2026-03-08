#include "knife_changer.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "core/memory/memory_manager.h"
#include "core/sdk/offsets.h"
#include "knife_changer_config.h"
#include <iomanip>
#include <iostream>

namespace Features {

std::unordered_map<int, uint64_t> KnifeChanger::s_modelHandleCache;

void KnifeChanger::Update() {
  if (!Config::Settings.knifeChanger.enabled) {
    return;
  }

  if (m_cachedModelIndex != Config::Settings.knifeChanger.knifeModel) {
    uint64_t foundHandle =
        ScanForModelHandle(Config::Settings.knifeChanger.knifeModel);
    if (foundHandle != 0) {
      s_modelHandleCache[Config::Settings.knifeChanger.knifeModel] =
          foundHandle;
    }
    m_cachedModelIndex = Config::Settings.knifeChanger.knifeModel;
  }

  ApplyKnife();
}

uint64_t KnifeChanger::ScanForModelHandle(int targetDefIndex) {
  return Core::GameManager::FindModelHandleByDefIndex(targetDefIndex);
}

bool KnifeChanger::IsKnifeIndex(int defIndex) const {
  switch (defIndex) {
  case 42: // default CT knife
  case 59: // default T knife
  case KNIFE_BAYONET:
  case KNIFE_FLIP:
  case KNIFE_GUT:
  case KNIFE_KARAMBIT:
  case KNIFE_M9_BAYONET:
  case KNIFE_HUNTSMAN:
  case KNIFE_FALCHION:
  case KNIFE_BOWIE:
  case KNIFE_BUTTERFLY:
  case KNIFE_SHADOW_DAGGERS:
  case KNIFE_PARACORD:
  case KNIFE_SURVIVAL:
  case KNIFE_URSUS:
  case KNIFE_NAVAJA:
  case KNIFE_STILETTO:
  case KNIFE_TALON:
  case KNIFE_CLASSIC:
  case KNIFE_SKELETON:
    return true;
  default:
    return false;
  }
}

void KnifeChanger::ApplyKnife() {
  if (m_pendingRestore) {
    uintptr_t ws = Core::GameManager::GetWeaponServices(
        Core::GameManager::GetLocalPlayerPawn());
    if (ws) {
      Core::MemoryManager::Write<uint32_t>(ws + SDK::Offsets::m_hActiveWeapon,
                                           m_pendingHandle);
    }
    m_pendingRestore = false;
    return;
  }

  uintptr_t localPawn = Core::GameManager::GetLocalPlayerPawn();
  if (!localPawn)
    return;

  uintptr_t weaponServices = Core::GameManager::GetWeaponServices(localPawn);
  if (!weaponServices)
    return;

  uint32_t activeWeaponHandle =
      Core::GameManager::GetActiveWeaponHandle(weaponServices);
  if (!activeWeaponHandle || activeWeaponHandle == 0xFFFFFFFF)
    return;

  static int unchangedCounter = 0;
  if (activeWeaponHandle == m_lastWeaponHandle) {
    unchangedCounter++;
    if (unchangedCounter >= 500) {
      unchangedCounter = 0;
    }
    return;
  }
  unchangedCounter = 0;

  uintptr_t activeWeapon =
      Core::GameManager::GetEntityFromHandle(activeWeaponHandle);
  if (!activeWeapon)
    return;

  short defIndex =
      Core::GameManager::GetEntityItemDefinitionIndex(activeWeapon);
  bool isKnife = IsKnifeIndex(defIndex);

  if (isKnife) {
    auto it = s_modelHandleCache.find(Config::Settings.knifeChanger.knifeModel);
    if (it != s_modelHandleCache.end() && it->second != 0) {
      uintptr_t myGameSceneNode =
          Core::GameManager::GetEntityGameSceneNode(activeWeapon);
      if (myGameSceneNode) {
        Core::MemoryManager::Write<uint64_t>(myGameSceneNode +
                                                 SDK::Offsets::m_modelState +
                                                 SDK::Offsets::m_hModel,
                                             it->second);
      }
    }

    uintptr_t econItemView =
        activeWeapon + SDK::Offsets::m_AttributeManager + SDK::Offsets::m_Item;
    // local visual only, not networked
    Core::MemoryManager::Write<int32_t>(
        econItemView + SDK::Offsets::m_iItemIDHigh, -1);
    Core::MemoryManager::Write<int32_t>(activeWeapon +
                                            SDK::Offsets::m_nFallbackPaintKit,
                                        Config::Settings.knifeChanger.paintKit);
    Core::MemoryManager::Write<float>(activeWeapon +
                                          SDK::Offsets::m_flFallbackWear,
                                      Config::Settings.knifeChanger.wear);
    Core::MemoryManager::Write<int32_t>(activeWeapon +
                                            SDK::Offsets::m_nFallbackSeed,
                                        Config::Settings.knifeChanger.seed);

    // Frame 1: write invalid handle to force engine to drop weapon
    Core::MemoryManager::Write<uint32_t>(weaponServices +
                                             SDK::Offsets::m_hActiveWeapon,
                                         0xFFFFFFFF); // force engine refresh
    m_pendingRestore = true;
    m_pendingHandle = activeWeaponHandle;
    m_lastWeaponHandle = 0; // reset so next frame sees handle as changed
  } else {
    m_lastWeaponHandle = activeWeaponHandle;
  }
}

} // namespace Features
