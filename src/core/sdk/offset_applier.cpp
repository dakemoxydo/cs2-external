#include "offset_applier.h"
#include "offsets.h"
#include <iostream>

namespace SDK {

void OffsetApplier::Apply(const OffsetSet& parsed) {
    Offsets::Publish(parsed);
}

bool OffsetApplier::Validate(const OffsetSet& parsed) const {
    return parsed.HasRequired();
}

void OffsetApplier::LogStatus(const OffsetSet& parsed) const {
    auto log = [](const char* name, ptrdiff_t val) {
        if (val != 0)
            std::cout << "  [+] " << name << " = 0x" << std::hex << val << std::dec << "\n";
        else
            std::cout << "  [!] " << name << " = 0 (MISSING)\n";
    };

    std::cout << "[+] Offsets loaded:\n";
    log("dwEntityList", parsed.dwEntityList);
    log("dwLocalPlayerPawn", parsed.dwLocalPlayerPawn);
    log("dwLocalPlayerController", parsed.dwLocalPlayerController);
    log("dwViewMatrix", parsed.dwViewMatrix);
    log("dwPlantedC4", parsed.dwPlantedC4);
    log("dwGlobalVars", parsed.dwGlobalVars);
    log("m_iHealth", parsed.m_iHealth);
    log("m_iTeamNum", parsed.m_iTeamNum);
    log("m_vOldOrigin", parsed.m_vOldOrigin);
    log("m_pGameSceneNode", parsed.m_pGameSceneNode);
    log("m_modelState", parsed.m_modelState);
    log("m_hPlayerPawn", parsed.m_hPlayerPawn);
    log("m_iszPlayerName", parsed.m_iszPlayerName);
    log("m_pClippingWeapon", parsed.m_pClippingWeapon);
    log("m_vecViewOffset", parsed.m_vecViewOffset);
    log("m_flSimulationTime", parsed.m_flSimulationTime);
    log("m_iIDEntIndex", parsed.m_iIDEntIndex);
    log("m_bIsScoped", parsed.m_bIsScoped);
    log("m_flC4Blow", parsed.m_flC4Blow);
    log("m_bBeingDefused", parsed.m_bBeingDefused);
    log("m_flDefuseCountDown", parsed.m_flDefuseCountDown);

    if (!Validate(parsed)) {
        std::cout << "[!] CRITICAL: Some essential offsets are missing! ESP may not work.\n";
    }
}

} // namespace SDK
