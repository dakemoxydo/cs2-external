#include "offset_applier.h"
#include "offsets.h"
#include <iostream>

namespace SDK {

void OffsetApplier::Apply(const OffsetParser::ParsedOffsets& parsed) {
    Offsets::dwEntityList = parsed.dwEntityList;
    Offsets::dwLocalPlayerPawn = parsed.dwLocalPlayerPawn;
    Offsets::dwLocalPlayerController = parsed.dwLocalPlayerController;
    Offsets::dwViewMatrix = parsed.dwViewMatrix;
    Offsets::dwPlantedC4 = parsed.dwPlantedC4;
    Offsets::m_fFlags = parsed.m_fFlags;
    Offsets::m_vecVelocity = parsed.m_vecVelocity;
    Offsets::m_iHealth = parsed.m_iHealth;
    Offsets::m_iTeamNum = parsed.m_iTeamNum;
    Offsets::m_vOldOrigin = parsed.m_vOldOrigin;
    Offsets::m_pGameSceneNode = parsed.m_pGameSceneNode;
    Offsets::m_modelState = parsed.m_modelState;
    Offsets::m_hPlayerPawn = parsed.m_hPlayerPawn;
    Offsets::m_iszPlayerName = parsed.m_iszPlayerName;
    Offsets::m_pClippingWeapon = parsed.m_pClippingWeapon;
    Offsets::m_vecViewOffset = parsed.m_vecViewOffset;
    Offsets::m_angEyeAngles = parsed.m_angEyeAngles;
    Offsets::m_aimPunchAngle = parsed.m_aimPunchAngle;
    Offsets::m_iIDEntIndex = parsed.m_iIDEntIndex;
    Offsets::m_bIsScoped = parsed.m_bIsScoped;
    Offsets::m_iShotsFired = parsed.m_iShotsFired;
    Offsets::m_nBombSite = parsed.m_nBombSite;
    Offsets::m_bBombTicking = parsed.m_bBombTicking;
    Offsets::m_flTimerLength = parsed.m_flTimerLength;
}

bool OffsetApplier::Validate() const {
    return Offsets::dwEntityList != 0
        && Offsets::dwLocalPlayerPawn != 0
        && Offsets::dwViewMatrix != 0;
}

void OffsetApplier::LogStatus() const {
    auto log = [](const char* name, ptrdiff_t val) {
        if (val != 0)
            std::cout << "  [+] " << name << " = 0x" << std::hex << val << std::dec << "\n";
        else
            std::cout << "  [!] " << name << " = 0 (MISSING)\n";
    };

    std::cout << "[+] Offsets loaded:\n";
    log("dwEntityList", Offsets::dwEntityList);
    log("dwLocalPlayerPawn", Offsets::dwLocalPlayerPawn);
    log("dwLocalPlayerController", Offsets::dwLocalPlayerController);
    log("dwViewMatrix", Offsets::dwViewMatrix);
    log("dwPlantedC4", Offsets::dwPlantedC4);
    log("m_iHealth", Offsets::m_iHealth);
    log("m_iTeamNum", Offsets::m_iTeamNum);
    log("m_vOldOrigin", Offsets::m_vOldOrigin);
    log("m_pGameSceneNode", Offsets::m_pGameSceneNode);
    log("m_modelState", Offsets::m_modelState);
    log("m_hPlayerPawn", Offsets::m_hPlayerPawn);
    log("m_iszPlayerName", Offsets::m_iszPlayerName);
    log("m_pClippingWeapon", Offsets::m_pClippingWeapon);
    log("m_vecViewOffset", Offsets::m_vecViewOffset);
    log("m_iIDEntIndex", Offsets::m_iIDEntIndex);
    log("m_bIsScoped", Offsets::m_bIsScoped);

    if (!Validate()) {
        std::cout << "[!] CRITICAL: Some essential offsets are missing! ESP may not work.\n";
    }
}

} // namespace SDK
