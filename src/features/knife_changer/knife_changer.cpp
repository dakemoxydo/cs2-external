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
  std::cout << "[KnifeChanger] update() called, enabled="
            << Config::Settings.knifeChanger.enabled
            << ", config: model=" << Config::Settings.knifeChanger.knifeModel
            << " kit=" << Config::Settings.knifeChanger.paintKit
            << " wear=" << Config::Settings.knifeChanger.wear
            << " seed=" << Config::Settings.knifeChanger.seed << std::endl;

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
  uintptr_t entityList = Core::GameManager::GetEntityList();
  if (!entityList)
    return 0;

  for (int i = 65; i <= 2048; i++) {
    uintptr_t listEntry = Core::MemoryManager::Read<uintptr_t>(
        entityList + 0x10 + 0x8 * (i >> 9));
    if (!listEntry)
      continue;

    uintptr_t entity =
        Core::MemoryManager::Read<uintptr_t>(listEntry + 0x70 * (i & 0x1FF));
    if (!entity)
      continue;

    uintptr_t econItemView =
        entity + SDK::Offsets::m_AttributeManager + SDK::Offsets::m_Item;
    short defIndex = Core::MemoryManager::Read<short>(
        econItemView + SDK::Offsets::m_iItemDefinitionIndex);

    if (defIndex == targetDefIndex) {
      uintptr_t gameSceneNode = Core::MemoryManager::Read<uintptr_t>(
          entity + SDK::Offsets::m_pGameSceneNode);
      if (gameSceneNode) {
        uint64_t handle = Core::MemoryManager::Read<uint64_t>(
            gameSceneNode + SDK::Offsets::m_modelState +
            SDK::Offsets::m_hModel);
        if (handle) {
          std::cout << "[KnifeChanger] Scan found handle 0x" << std::hex
                    << handle << std::dec << " for defIndex " << targetDefIndex
                    << std::endl;
          return handle;
        }
      }
    }
  }
  return 0;
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
  // Frame 2: restore real handle on next call
  if (m_pendingRestore) {
    uintptr_t ws = Core::MemoryManager::Read<uintptr_t>(
        Core::GameManager::GetLocalPlayerPawn() +
        SDK::Offsets::m_pWeaponServices);
    if (ws) {
      Core::MemoryManager::Write<uint32_t>(ws + SDK::Offsets::m_hActiveWeapon,
                                           m_pendingHandle);
    }
    m_pendingRestore = false;
    return;
  }

  uintptr_t localPawn = Core::GameManager::GetLocalPlayerPawn();
  std::cout << std::hex << "[KnifeChanger] localPawn = 0x" << localPawn
            << std::dec << std::endl;
  if (!localPawn) {
    std::cout << "[KnifeChanger] ERROR: localPawn is null, aborting"
              << std::endl;
    return;
  }

  // Pawn -> Services
  std::cout << "[KnifeChanger] m_pWeaponServices OFFSET = 0x" << std::hex
            << SDK::Offsets::m_pWeaponServices << std::dec << std::endl;
  uintptr_t weaponServices = Core::MemoryManager::Read<uintptr_t>(
      localPawn + SDK::Offsets::m_pWeaponServices);
  std::cout << std::hex << "[KnifeChanger] weaponServices = 0x"
            << weaponServices << std::dec << std::endl;
  if (!weaponServices) {
    std::cout << "[KnifeChanger] ERROR: weaponServices is null, aborting"
              << std::endl;
    return;
  }

  // Services -> Handle
  uint32_t activeWeaponHandle = Core::MemoryManager::Read<uint32_t>(
      weaponServices + SDK::Offsets::m_hActiveWeapon);
  std::cout << std::hex << "[KnifeChanger] activeWeaponHandle = 0x"
            << activeWeaponHandle << std::dec << std::endl;
  if (!activeWeaponHandle || activeWeaponHandle == 0xFFFFFFFF) {
    std::cout << "[KnifeChanger] ERROR: activeWeaponHandle is null, aborting"
              << std::endl;
    return;
  }

  // Write trigger: ONLY when active weapon handle differs from cached last
  // handle
  static int unchangedCounter = 0;
  if (activeWeaponHandle == m_lastWeaponHandle) {
    unchangedCounter++;
    if (unchangedCounter >= 500) {
      std::cout << std::hex << "[KnifeChanger] lastHandle=0x"
                << m_lastWeaponHandle << " currentHandle=0x"
                << activeWeaponHandle << std::dec << " changed=false"
                << std::endl;
      std::cout << "[KnifeChanger] handle unchanged, skipping write"
                << std::endl;
      unchangedCounter = 0;
    }
    return;
  }
  unchangedCounter = 0;
  std::cout << std::hex << "[KnifeChanger] lastHandle=0x" << m_lastWeaponHandle
            << " currentHandle=0x" << activeWeaponHandle << std::dec
            << " changed=true" << std::endl;

  // Handle -> EntityList -> Weapon
  uintptr_t activeWeapon =
      Core::GameManager::GetEntityFromHandle(activeWeaponHandle);
  std::cout << std::hex << "[KnifeChanger] weaponEntity = 0x" << activeWeapon
            << std::dec << std::endl;
  if (!activeWeapon) {
    std::cout << "[KnifeChanger] ERROR: weaponEntity is null, aborting"
              << std::endl;
    return;
  }

  // Read definition index
  uintptr_t econItemView =
      activeWeapon + SDK::Offsets::m_AttributeManager + SDK::Offsets::m_Item;
  std::cout << std::hex << "[KnifeChanger] econItemView = 0x" << econItemView
            << std::dec << std::endl;
  if (!econItemView || econItemView == activeWeapon) {
    std::cout << "[KnifeChanger] ERROR: econItemView is null, aborting"
              << std::endl;
    return;
  }

  short defIndex = Core::MemoryManager::Read<short>(
      econItemView + SDK::Offsets::m_iItemDefinitionIndex);
  bool isKnife = IsKnifeIndex(defIndex);
  std::cout << "[KnifeChanger] defIndex=" << defIndex << std::endl;
  std::cout << "[KnifeChanger] isKnife=" << (isKnife ? "true" : "false")
            << std::endl;

  if (isKnife) {
    auto it = s_modelHandleCache.find(Config::Settings.knifeChanger.knifeModel);
    if (it != s_modelHandleCache.end() && it->second != 0) {
      uintptr_t myGameSceneNode = Core::MemoryManager::Read<uintptr_t>(
          activeWeapon + SDK::Offsets::m_pGameSceneNode);
      if (myGameSceneNode) {
        std::cout << "[KnifeChanger] writing m_hModel = 0x" << std::hex
                  << it->second << std::dec << std::endl;
        Core::MemoryManager::Write<uint64_t>(myGameSceneNode +
                                                 SDK::Offsets::m_modelState +
                                                 SDK::Offsets::m_hModel,
                                             it->second);
      }
    } else {
      std::cout << "[KnifeChanger] handle cache miss for defIndex="
                << Config::Settings.knifeChanger.knifeModel << ", skin only"
                << std::endl;
    }

    std::cout << std::hex << "[KnifeChanger] writing m_iItemIDHigh = -1 at 0x"
              << (econItemView + SDK::Offsets::m_iItemIDHigh) << std::dec
              << std::endl;
    Core::MemoryManager::Write<int32_t>(econItemView +
                                            SDK::Offsets::m_iItemIDHigh,
                                        -1); // local visual only, not networked

    std::cout << std::hex
              << "[KnifeChanger] writing m_nFallbackPaintKit = " << std::dec
              << Config::Settings.knifeChanger.paintKit << std::hex << " at 0x"
              << (activeWeapon + SDK::Offsets::m_nFallbackPaintKit) << std::dec
              << std::endl;
    Core::MemoryManager::Write<int32_t>(
        activeWeapon + SDK::Offsets::m_nFallbackPaintKit,
        Config::Settings.knifeChanger
            .paintKit); // local visual only, not networked

    std::cout << std::hex
              << "[KnifeChanger] writing m_flFallbackWear = " << std::dec
              << Config::Settings.knifeChanger.wear << std::hex << " at 0x"
              << (activeWeapon + SDK::Offsets::m_flFallbackWear) << std::dec
              << std::endl;
    Core::MemoryManager::Write<float>(
        activeWeapon + SDK::Offsets::m_flFallbackWear,
        Config::Settings.knifeChanger.wear); // local visual only, not networked

    std::cout << std::hex
              << "[KnifeChanger] writing m_nFallbackSeed = " << std::dec
              << Config::Settings.knifeChanger.seed << std::hex << " at 0x"
              << (activeWeapon + SDK::Offsets::m_nFallbackSeed) << std::dec
              << std::endl;
    Core::MemoryManager::Write<int32_t>(
        activeWeapon + SDK::Offsets::m_nFallbackSeed,
        Config::Settings.knifeChanger.seed); // local visual only, not networked

    std::cout << "[KnifeChanger] apply complete" << std::endl;
    // Frame 1: write invalid handle to force engine to drop weapon
    Core::MemoryManager::Write<uint32_t>(weaponServices +
                                             SDK::Offsets::m_hActiveWeapon,
                                         0xFFFFFFFF); // force engine refresh
    m_pendingRestore = true;
    m_pendingHandle = activeWeaponHandle;
    m_lastWeaponHandle = 0; // reset so next frame sees handle as changed
  } else {
    std::cout << "[KnifeChanger] not a knife, skipping" << std::endl;
    m_lastWeaponHandle = activeWeaponHandle;
  }
}

} // namespace Features
