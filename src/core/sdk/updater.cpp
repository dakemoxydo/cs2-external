#include "updater.h"
#include "offsets.h"
#include <atomic>
#include <iostream>
#include <regex>
#include <string>
#include <thread>
#include <windows.h>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")

namespace SDK {

// ─── HTTP helper ─────────────────────────────────────────────────────────────
static std::string FetchHTTP(const std::string &url) {
  HINTERNET hInt = InternetOpenA("CS2Overlay/1.0", INTERNET_OPEN_TYPE_PRECONFIG,
                                 nullptr, nullptr, 0);
  if (!hInt)
    return {};

  HINTERNET hUrl =
      InternetOpenUrlA(hInt, url.c_str(), nullptr, 0,
                       INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE, 0);
  if (!hUrl) {
    InternetCloseHandle(hInt);
    return {};
  }

  std::string result;
  char buf[8192];
  DWORD n = 0;
  while (InternetReadFile(hUrl, buf, sizeof(buf) - 1, &n) && n > 0) {
    buf[n] = '\0';
    result.append(buf, n);
  }
  InternetCloseHandle(hUrl);
  InternetCloseHandle(hInt);
  return result;
}

// ─── Parse a decimal or hex number from JSON ─────────────────────────────────
// Handles both: "key": 12345  and  "key": "0x3f00"
static ptrdiff_t ParseOffset(const std::string &src, const std::string &key) {
  {
    std::regex re("\"" + key + "\"\\s*:\\s*(\\d+)");
    std::smatch m;
    if (std::regex_search(src, m, re))
      return static_cast<ptrdiff_t>(std::stoull(m[1].str()));
  }
  {
    std::regex re("\"" + key + "\"\\s*:\\s*\"0x([0-9a-fA-F]+)\"");
    std::smatch m;
    if (std::regex_search(src, m, re))
      return static_cast<ptrdiff_t>(std::stoull(m[1].str(), nullptr, 16));
  }
  return 0;
}

// ─── Apply fetched JSON to Offsets namespace ─────────────────────────────────
static void ApplyOffsets(const std::string &offsetsJson,
                         const std::string &clientJson) {
  // Engine-level pointers
  Offsets::dwEntityList = ParseOffset(offsetsJson, "dwEntityList");
  Offsets::dwLocalPlayerPawn = ParseOffset(offsetsJson, "dwLocalPlayerPawn");
  Offsets::dwViewMatrix = ParseOffset(offsetsJson, "dwViewMatrix");
  Offsets::dwPlantedC4 = ParseOffset(offsetsJson, "dwPlantedC4");

  // Member offsets
  Offsets::m_iHealth = ParseOffset(clientJson, "m_iHealth");
  Offsets::m_iTeamNum = ParseOffset(clientJson, "m_iTeamNum");
  Offsets::m_vOldOrigin = ParseOffset(clientJson, "m_vOldOrigin");
  Offsets::m_pGameSceneNode = ParseOffset(clientJson, "m_pGameSceneNode");
  Offsets::m_modelState = ParseOffset(clientJson, "m_modelState");
  Offsets::m_hPlayerPawn = ParseOffset(clientJson, "m_hPlayerPawn");
  Offsets::m_iszPlayerName = ParseOffset(clientJson, "m_iszPlayerName");
  Offsets::m_pClippingWeapon = ParseOffset(clientJson, "m_pClippingWeapon");

  // Knife Changer & Weapons
  Offsets::m_pWeaponServices = ParseOffset(clientJson, "m_pWeaponServices");
  Offsets::m_hActiveWeapon = ParseOffset(clientJson, "m_hActiveWeapon");
  Offsets::m_AttributeManager = ParseOffset(clientJson, "m_AttributeManager");
  Offsets::m_Item = ParseOffset(clientJson, "m_Item");
  Offsets::m_iItemDefinitionIndex =
      ParseOffset(clientJson, "m_iItemDefinitionIndex");
  Offsets::m_hModel = ParseOffset(clientJson, "m_hModel");
  Offsets::m_iItemIDHigh = ParseOffset(clientJson, "m_iItemIDHigh");
  Offsets::m_nFallbackPaintKit = ParseOffset(clientJson, "m_nFallbackPaintKit");
  Offsets::m_flFallbackWear = ParseOffset(clientJson, "m_flFallbackWear");
  Offsets::m_nFallbackSeed = ParseOffset(clientJson, "m_nFallbackSeed");

  // Aimbot / Triggerbot
  Offsets::m_angEyeAngles = ParseOffset(clientJson, "m_angEyeAngles");
  Offsets::m_iCrosshairEntityHandle =
      ParseOffset(clientJson, "m_iCrosshairEntityHandle");
  Offsets::m_bIsScoped = ParseOffset(clientJson, "m_bIsScoped");
  Offsets::m_iShotsFired = ParseOffset(clientJson, "m_iShotsFired");

  // Bomb
  Offsets::m_nBombSite = ParseOffset(clientJson, "m_nBombSite");
  Offsets::m_bBombTicking = ParseOffset(clientJson, "m_bBombTicking");
  Offsets::m_flTimerLength = ParseOffset(clientJson, "m_flTimerLength");

  // Fallback hardcoded values if dumper returns 0
  if (Offsets::m_angEyeAngles == 0)
    Offsets::m_angEyeAngles = 0x3DD0;
  if (Offsets::m_iCrosshairEntityHandle == 0)
    Offsets::m_iCrosshairEntityHandle = 0x13D8;
  if (Offsets::m_bIsScoped == 0)
    Offsets::m_bIsScoped = 0x1404;
  if (Offsets::m_iShotsFired == 0)
    Offsets::m_iShotsFired = 0x270C;

  std::cout << std::hex << "[INFO] Offsets loaded:\n"
            << "  dwEntityList:             0x" << Offsets::dwEntityList << "\n"
            << "  dwLocalPlayerPawn:        0x" << Offsets::dwLocalPlayerPawn
            << "\n"
            << "  dwViewMatrix:             0x" << Offsets::dwViewMatrix << "\n"
            << "  dwPlantedC4:              0x" << Offsets::dwPlantedC4 << "\n"
            << "  m_angEyeAngles:           0x" << Offsets::m_angEyeAngles
            << "\n"
            << "  m_iCrosshairEntityHandle: 0x"
            << Offsets::m_iCrosshairEntityHandle << "\n"
            << "  m_bIsScoped:              0x" << Offsets::m_bIsScoped << "\n"
            << "  m_iShotsFired:            0x" << Offsets::m_iShotsFired
            << "\n"
            << std::dec;
}

// ─── Background fetch thread ─────────────────────────────────────────────────
static std::atomic<bool> s_fetchDone{false};

static void FetchThreadProc() {
  const std::string BASE =
      "https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/";
  std::string offsetsJson = FetchHTTP(BASE + "offsets.json");
  std::string clientJson = FetchHTTP(BASE + "client_dll.json");

  if (offsetsJson.empty() || clientJson.empty()) {
    std::cerr
        << "[ERROR] Failed to fetch offsets from GitHub. Using defaults.\n";
    s_fetchDone = true;
    return;
  }

  if (ParseOffset(offsetsJson, "dwEntityList") == 0) {
    std::cerr
        << "[ERROR] Failed to parse dwEntityList — keeping current values.\n";
    s_fetchDone = true;
    return;
  }

  ApplyOffsets(offsetsJson, clientJson);
  s_fetchDone = true;
}

// ─── Public: kick off async update, returns immediately ──────────────────────
bool Updater::UpdateOffsets() {
  std::cout << "[INFO] Launching async offset update from a2x/cs2-dumper...\n";
  std::thread(FetchThreadProc).detach();
  return true; // Main thread continues with default/last offsets immediately
}

} // namespace SDK
