#include "offset_parser.h"
#include <iostream>
#include <limits>
#include <regex>
#include <unordered_map>

namespace SDK {

namespace {
ptrdiff_t ParseOffsetValue(const nlohmann::json &value) {
    uint64_t parsed = 0;
    if (value.is_string()) {
        parsed = std::stoull(value.get<std::string>(), nullptr, 0);
    } else if (value.is_number_unsigned()) {
        parsed = value.get<uint64_t>();
    } else if (value.is_number_integer() && value.get<int64_t>() >= 0) {
        parsed = static_cast<uint64_t>(value.get<int64_t>());
    } else {
        return 0;
    }
    if (parsed > static_cast<uint64_t>((std::numeric_limits<ptrdiff_t>::max)())) {
        return 0;
    }
    return static_cast<ptrdiff_t>(parsed);
}
}

ptrdiff_t OffsetParser::GetVal(const nlohmann::json& j, const std::string& key) {
    if (!j.contains(key)) return 0;
    return ParseOffsetValue(j[key]);
}

ptrdiff_t OffsetParser::FindField(const nlohmann::json& clientJson,
                                  std::initializer_list<const char*> classNames,
                                  const std::string& fieldName) {
    if (!clientJson.contains("client.dll") || !clientJson["client.dll"].contains("classes")) return 0;
    const auto& classes = clientJson["client.dll"]["classes"];
    for (const char* className : classNames) {
        const auto classIt = classes.find(className);
        if (classIt != classes.end() && classIt->contains("fields") &&
            (*classIt)["fields"].contains(fieldName)) {
            return ParseOffsetValue((*classIt)["fields"][fieldName]);
        }
    }
    return 0;
}

OffsetSet OffsetParser::ParseJson(const std::string& offsetsJson, const std::string& clientJson) {
    OffsetSet result;
    try {
        auto offsetsParsed = nlohmann::json::parse(offsetsJson);
        auto clientParsed = nlohmann::json::parse(clientJson);

        if (offsetsParsed.contains("client.dll")) {
            const auto& c = offsetsParsed["client.dll"];
            result.dwEntityList = GetVal(c, "dwEntityList");
            result.dwLocalPlayerPawn = GetVal(c, "dwLocalPlayerPawn");
            result.dwLocalPlayerController = GetVal(c, "dwLocalPlayerController");
            result.dwViewMatrix = GetVal(c, "dwViewMatrix");
            result.dwPlantedC4 = GetVal(c, "dwPlantedC4");
            result.dwGlobalVars = GetVal(c, "dwGlobalVars");
        }

        const auto baseEntity = {"C_BaseEntity", "CBaseEntity"};
        const auto playerPawn = {"C_CSPlayerPawn", "CCSPlayerPawn"};
        const auto controller = {"CCSPlayerController", "CBasePlayerController"};
        result.m_fFlags = FindField(clientParsed, playerPawn, "m_fFlags");
        if (!result.m_fFlags) result.m_fFlags = FindField(clientParsed, baseEntity, "m_fFlags");
        result.m_vecVelocity = FindField(clientParsed, playerPawn, "m_vecVelocity");
        if (!result.m_vecVelocity) result.m_vecVelocity = FindField(clientParsed, baseEntity, "m_vecVelocity");
        result.m_iHealth = FindField(clientParsed, baseEntity, "m_iHealth");
        result.m_iTeamNum = FindField(clientParsed, baseEntity, "m_iTeamNum");
        result.m_vOldOrigin = FindField(clientParsed,
                                        {"C_BasePlayerPawn", "CBasePlayerPawn"},
                                        "m_vOldOrigin");
        result.m_pGameSceneNode = FindField(clientParsed, baseEntity, "m_pGameSceneNode");
        result.m_modelState = FindField(clientParsed, {"CSkeletonInstance"}, "m_modelState");
        result.m_hPlayerPawn = FindField(clientParsed, controller, "m_hPlayerPawn");
        result.m_iszPlayerName = FindField(clientParsed, controller, "m_iszPlayerName");
        result.m_bIsLocalPlayerController = FindField(clientParsed, {"CBasePlayerController", "CCSPlayerController"}, "m_bIsLocalPlayerController");
        result.m_pClippingWeapon = FindField(clientParsed, playerPawn, "m_pClippingWeapon");
        result.m_vecViewOffset = FindField(clientParsed, playerPawn, "m_vecViewOffset");
        if (!result.m_vecViewOffset) result.m_vecViewOffset = FindField(clientParsed, {"CBaseModelEntity"}, "m_vecViewOffset");
        result.m_flSimulationTime = FindField(clientParsed, baseEntity, "m_flSimulationTime");
        result.m_angEyeAngles = FindField(clientParsed, playerPawn, "m_angEyeAngles");
        result.m_aimPunchAngle = FindField(clientParsed, playerPawn, "m_aimPunchAngle");
        result.m_iIDEntIndex = FindField(clientParsed, playerPawn, "m_iIDEntIndex");
        result.m_bIsScoped = FindField(clientParsed, playerPawn, "m_bIsScoped");
        result.m_iShotsFired = FindField(clientParsed, playerPawn, "m_iShotsFired");
        result.m_angShootAngleHistory = FindField(clientParsed, playerPawn, "m_angShootAngleHistory");
        result.m_flRange = FindField(clientParsed, {"CCSWeaponBaseVData", "CCSWeaponBase"}, "m_flRange");
        result.m_pBulletServices = FindField(clientParsed, playerPawn, "m_pBulletServices");
        result.m_entitySpottedState = FindField(clientParsed, playerPawn, "m_entitySpottedState");
        result.m_bSpottedByMask = FindField(clientParsed, {"EntitySpottedState_t"}, "m_bSpottedByMask");
        result.m_nBombSite = FindField(clientParsed, {"C_PlantedC4"}, "m_nBombSite");
        result.m_bBombTicking = FindField(clientParsed, {"C_PlantedC4"}, "m_bBombTicking");
        result.m_flTimerLength = FindField(clientParsed, {"C_PlantedC4"}, "m_flTimerLength");
        result.m_flC4Blow = FindField(clientParsed, {"C_PlantedC4"}, "m_flC4Blow");
        result.m_bBeingDefused = FindField(clientParsed, {"C_PlantedC4"}, "m_bBeingDefused");
        result.m_flDefuseCountDown = FindField(clientParsed, {"C_PlantedC4"}, "m_flDefuseCountDown");
    } catch (const std::exception& e) {
        std::cerr << "[!] JSON parse error: " << e.what() << "\n";
    }
    return result;
}

static ptrdiff_t ParseHppValue(const std::string& content, const std::string& macroName) {
    std::regex re(R"(#define\s+)" + macroName + R"(\s+(0x[0-9a-fA-F]+|\d+))");
    std::smatch match;
    if (std::regex_search(content, match, re)) {
        try {
            return static_cast<ptrdiff_t>(std::stoull(match[1].str(), nullptr, 0));
        } catch (...) {}
    }
    return 0;
}

OffsetSet OffsetParser::ParseHpp(const std::string& offsetsHpp, const std::string& clientHpp) {
    OffsetSet result;
    // Parse offsets.hpp for dw* values
    if (!offsetsHpp.empty()) {
        result.dwEntityList = ParseHppValue(offsetsHpp, "client_dwEntityList");
        result.dwLocalPlayerPawn = ParseHppValue(offsetsHpp, "client_dwLocalPlayerPawn");
        result.dwLocalPlayerController = ParseHppValue(offsetsHpp, "client_dwLocalPlayerController");
        result.dwViewMatrix = ParseHppValue(offsetsHpp, "client_dwViewMatrix");
        result.dwPlantedC4 = ParseHppValue(offsetsHpp, "client_dwPlantedC4");
        result.dwGlobalVars = ParseHppValue(offsetsHpp, "client_dwGlobalVars");
    }
    // Parse client_dll.hpp for field values
    if (!clientHpp.empty()) {
        result.m_fFlags = ParseHppValue(clientHpp, "CCSPlayerPawn_m_fFlags");
        if (result.m_fFlags == 0) result.m_fFlags = ParseHppValue(clientHpp, "CBaseEntity_m_fFlags");
        result.m_vecVelocity = ParseHppValue(clientHpp, "CCSPlayerPawn_m_vecVelocity");
        if (result.m_vecVelocity == 0) result.m_vecVelocity = ParseHppValue(clientHpp, "CBaseEntity_m_vecVelocity");
        result.m_iHealth = ParseHppValue(clientHpp, "CBaseEntity_m_iHealth");
        result.m_iTeamNum = ParseHppValue(clientHpp, "CBaseEntity_m_iTeamNum");
        result.m_vOldOrigin = ParseHppValue(clientHpp, "C_BasePlayerPawn_m_vOldOrigin");
        if (result.m_vOldOrigin == 0) result.m_vOldOrigin = ParseHppValue(clientHpp, "CBasePlayerPawn_m_vOldOrigin");
        result.m_pGameSceneNode = ParseHppValue(clientHpp, "CBaseEntity_m_pGameSceneNode");
        result.m_modelState = ParseHppValue(clientHpp, "CBaseEntity_m_modelState");
        result.m_hPlayerPawn = ParseHppValue(clientHpp, "CCSPlayerController_m_hPlayerPawn");
        result.m_iszPlayerName = ParseHppValue(clientHpp, "CCSPlayerController_m_iszPlayerName");
        result.m_bIsLocalPlayerController = ParseHppValue(clientHpp, "CBasePlayerController_m_bIsLocalPlayerController");
        result.m_pClippingWeapon = ParseHppValue(clientHpp, "CCSPlayerPawn_m_pClippingWeapon");
        result.m_vecViewOffset = ParseHppValue(clientHpp, "CCSPlayerPawn_m_vecViewOffset");
        if (result.m_vecViewOffset == 0) result.m_vecViewOffset = ParseHppValue(clientHpp, "CBaseModelEntity_m_vecViewOffset");
        result.m_flSimulationTime = ParseHppValue(clientHpp, "C_BaseEntity_m_flSimulationTime");
        if (result.m_flSimulationTime == 0) result.m_flSimulationTime = ParseHppValue(clientHpp, "CBaseEntity_m_flSimulationTime");
        result.m_angEyeAngles = ParseHppValue(clientHpp, "CCSPlayerPawn_m_angEyeAngles");
        result.m_aimPunchAngle = ParseHppValue(clientHpp, "CCSPlayerPawn_m_aimPunchAngle");
        result.m_iIDEntIndex = ParseHppValue(clientHpp, "CCSPlayerPawn_m_iIDEntIndex");
        result.m_bIsScoped = ParseHppValue(clientHpp, "CCSPlayerPawn_m_bIsScoped");
        result.m_iShotsFired = ParseHppValue(clientHpp, "CCSPlayerPawn_m_iShotsFired");
        result.m_angShootAngleHistory = ParseHppValue(clientHpp, "CCSPlayerPawn_m_angShootAngleHistory");
        result.m_flRange = ParseHppValue(clientHpp, "CCSWeaponBaseVData_m_flRange");
        if (result.m_flRange == 0) result.m_flRange = ParseHppValue(clientHpp, "CCSWeaponBase_m_flRange");
        result.m_pBulletServices = ParseHppValue(clientHpp, "CCSPlayerPawn_m_pBulletServices");
        result.m_entitySpottedState = ParseHppValue(clientHpp, "C_CSPlayerPawn_m_entitySpottedState");
        if (result.m_entitySpottedState == 0) result.m_entitySpottedState = ParseHppValue(clientHpp, "CCSPlayerPawn_m_entitySpottedState");
        result.m_bSpottedByMask = ParseHppValue(clientHpp, "EntitySpottedState_t_m_bSpottedByMask");
        result.m_nBombSite = ParseHppValue(clientHpp, "C_PlantedC4_m_nBombSite");
        result.m_bBombTicking = ParseHppValue(clientHpp, "C_PlantedC4_m_bBombTicking");
        result.m_flTimerLength = ParseHppValue(clientHpp, "C_PlantedC4_m_flTimerLength");
        result.m_flC4Blow = ParseHppValue(clientHpp, "C_PlantedC4_m_flC4Blow");
        result.m_bBeingDefused = ParseHppValue(clientHpp, "C_PlantedC4_m_bBeingDefused");
        result.m_flDefuseCountDown = ParseHppValue(clientHpp, "C_PlantedC4_m_flDefuseCountDown");
    }
    return result;
}

OffsetSet OffsetParser::Parse(const OffsetFileLoader::FileResult& files) {
    if (files.hasJson()) {
        return ParseJson(files.offsetsJson, files.clientJson);
    }
    if (files.hasHpp()) {
        return ParseHpp(files.offsetsHpp, files.clientHpp);
    }
    return OffsetSet();
}

} // namespace SDK
