#include "updater.h"
#include "offsets.h"
#include <iostream>
#include <regex>
#include <string>
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

// ─── Parse a decimal or hex number from JSON/header ──────────────────────────
// Handles both: "key": 12345  and  "key": "0x3f00"
static ptrdiff_t ParseOffset(const std::string &src, const std::string &key) {
  // Try decimal: "key": 12345
  {
    std::regex re("\"" + key + "\"\\s*:\\s*(\\d+)");
    std::smatch m;
    if (std::regex_search(src, m, re))
      return static_cast<ptrdiff_t>(std::stoull(m[1].str()));
  }
  // Try hex string: "key": "0x1234abcd"
  {
    std::regex re("\"" + key + "\"\\s*:\\s*\"0x([0-9a-fA-F]+)\"");
    std::smatch m;
    if (std::regex_search(src, m, re))
      return static_cast<ptrdiff_t>(std::stoull(m[1].str(), nullptr, 16));
  }
  return 0;
}

// ─── UpdateOffsets
// ────────────────────────────────────────────────────────────
bool Updater::UpdateOffsets() {
  std::cout << "[INFO] Fetching latest offsets from a2x/cs2-dumper...\n";

  const std::string BASE =
      "https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/";
  std::string offsetsJson = FetchHTTP(BASE + "offsets.json");
  std::string clientJson = FetchHTTP(BASE + "client_dll.json");

  if (offsetsJson.empty() || clientJson.empty()) {
    std::cerr << "[ERROR] Failed to fetch offsets from GitHub.\n";
    return false;
  }

  // ── offsets.json (engine-level pointers) ──────────────────────────────────
  Offsets::dwEntityList = ParseOffset(offsetsJson, "dwEntityList");
  Offsets::dwLocalPlayerPawn = ParseOffset(offsetsJson, "dwLocalPlayerPawn");
  Offsets::dwViewMatrix = ParseOffset(offsetsJson, "dwViewMatrix");
  Offsets::dwPlantedC4 = ParseOffset(offsetsJson, "dwPlantedC4");

  // ── client_dll.json (member offsets) ──────────────────────────────────────
  Offsets::m_iHealth = ParseOffset(clientJson, "m_iHealth");
  Offsets::m_iTeamNum = ParseOffset(clientJson, "m_iTeamNum");
  Offsets::m_vOldOrigin = ParseOffset(clientJson, "m_vOldOrigin");
  Offsets::m_pGameSceneNode = ParseOffset(clientJson, "m_pGameSceneNode");
  Offsets::m_modelState = ParseOffset(clientJson, "m_modelState");
  Offsets::m_hPlayerPawn = ParseOffset(clientJson, "m_hPlayerPawn");
  Offsets::m_iszPlayerName = ParseOffset(clientJson, "m_iszPlayerName");

  // ── Aimbot / Triggerbot member offsets ────────────────────────────────────
  Offsets::m_angEyeAngles = ParseOffset(clientJson, "m_angEyeAngles");
  Offsets::m_iCrosshairEntityHandle =
      ParseOffset(clientJson, "m_iCrosshairEntityHandle");
  Offsets::m_bIsScoped = ParseOffset(clientJson, "m_bIsScoped");
  Offsets::m_iShotsFired = ParseOffset(clientJson, "m_iShotsFired");

  // ── Fallback hardcoded values if dumper returns 0 ─────────────────────────
  // Values from a2x cs2-dumper as of CS2 patch 1.40.x
  if (Offsets::m_angEyeAngles == 0)
    Offsets::m_angEyeAngles = 0x1510;
  if (Offsets::m_iCrosshairEntityHandle == 0)
    Offsets::m_iCrosshairEntityHandle = 0x13D8;
  if (Offsets::m_bIsScoped == 0)
    Offsets::m_bIsScoped = 0x1404;
  if (Offsets::m_iShotsFired == 0)
    Offsets::m_iShotsFired = 0x1368;

  if (Offsets::dwEntityList == 0) {
    std::cerr << "[ERROR] Failed to parse dwEntityList!\n";
    return false;
  }

  // ── Print summary ─────────────────────────────────────────────────────────
  std::cout << std::hex;
  std::cout << "[INFO] Offsets loaded:\n"
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
            << "\n";
  std::cout << std::dec;
  return true;
}

} // namespace SDK
