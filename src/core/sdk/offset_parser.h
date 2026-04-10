#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "offset_file_loader.h"

namespace SDK {

class OffsetParser {
public:
    struct ParsedOffsets {
        ptrdiff_t dwEntityList = 0;
        ptrdiff_t dwLocalPlayerPawn = 0;
        ptrdiff_t dwLocalPlayerController = 0;
        ptrdiff_t dwViewMatrix = 0;
        ptrdiff_t dwPlantedC4 = 0;
        ptrdiff_t m_fFlags = 0;
        ptrdiff_t m_vecVelocity = 0;
        ptrdiff_t m_iHealth = 0;
        ptrdiff_t m_iTeamNum = 0;
        ptrdiff_t m_vOldOrigin = 0;
        ptrdiff_t m_pGameSceneNode = 0;
        ptrdiff_t m_modelState = 0;
        ptrdiff_t m_hPlayerPawn = 0;
        ptrdiff_t m_iszPlayerName = 0;
        ptrdiff_t m_pClippingWeapon = 0;
        ptrdiff_t m_vecViewOffset = 0;
        ptrdiff_t m_angEyeAngles = 0;
        ptrdiff_t m_aimPunchAngle = 0;
        ptrdiff_t m_iIDEntIndex = 0;
        ptrdiff_t m_bIsScoped = 0;
        ptrdiff_t m_iShotsFired = 0;
        ptrdiff_t m_nBombSite = 0;
        ptrdiff_t m_bBombTicking = 0;
        ptrdiff_t m_flTimerLength = 0;

        int missingCount() const;
    };

    ParsedOffsets Parse(const OffsetFileLoader::FileResult& files);

private:
    ParsedOffsets ParseJson(const std::string& offsetsJson, const std::string& clientJson);
    ParsedOffsets ParseHpp(const std::string& offsetsHpp, const std::string& clientHpp);
    ptrdiff_t GetVal(const nlohmann::json& j, const std::string& key);
    ptrdiff_t FindField(const nlohmann::json& clientJson, const std::string& fieldName);
};

} // namespace SDK
