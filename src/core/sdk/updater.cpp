#include "updater.h"
#include "offsets.h"
#include <iostream>
#include <regex>
#include <string>
#include <windows.h>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")

namespace SDK {

std::string FetchHTTP(const std::string &url) {
  HINTERNET hInternet = InternetOpenA(
      "CS2Overlay/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
  if (!hInternet)
    return "";

  HINTERNET hUrl =
      InternetOpenUrlA(hInternet, url.c_str(), NULL, 0,
                       INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE, 0);
  if (!hUrl) {
    InternetCloseHandle(hInternet);
    return "";
  }

  std::string result;
  DWORD bytesRead = 0;
  char buffer[4096];
  while (InternetReadFile(hUrl, buffer, sizeof(buffer) - 1, &bytesRead) &&
         bytesRead > 0) {
    buffer[bytesRead] = '\0';
    result.append(buffer, bytesRead);
  }

  InternetCloseHandle(hUrl);
  InternetCloseHandle(hInternet);
  return result;
}

ptrdiff_t ParseOffset(const std::string &json, const std::string &key) {
  std::string pattern = "\"" + key + "\"\\s*:\\s*(\\d+)";
  std::regex re(pattern);
  std::smatch match;
  if (std::regex_search(json, match, re) && match.size() > 1) {
    return static_cast<ptrdiff_t>(std::stoull(match[1].str()));
  }
  return 0;
}

bool Updater::UpdateOffsets() {
  std::cout << "[INFO] Fetching latest offsets from a2x/cs2-dumper..."
            << std::endl;

  std::string offsetsJson = FetchHTTP("https://raw.githubusercontent.com/a2x/"
                                      "cs2-dumper/main/output/offsets.json");
  std::string clientJson = FetchHTTP("https://raw.githubusercontent.com/a2x/"
                                     "cs2-dumper/main/output/client_dll.json");

  if (offsetsJson.empty() || clientJson.empty()) {
    std::cerr << "[ERROR] Failed to fetch offsets from GitHub." << std::endl;
    return false;
  }

  // Parse offsets.json
  Offsets::dwEntityList = ParseOffset(offsetsJson, "dwEntityList");
  Offsets::dwLocalPlayerPawn = ParseOffset(offsetsJson, "dwLocalPlayerPawn");
  Offsets::dwViewMatrix = ParseOffset(offsetsJson, "dwViewMatrix");

  // Parse client_dll.json
  Offsets::m_iHealth = ParseOffset(clientJson, "m_iHealth");
  Offsets::m_iTeamNum = ParseOffset(clientJson, "m_iTeamNum");
  Offsets::m_vOldOrigin = ParseOffset(clientJson, "m_vOldOrigin");
  Offsets::m_pGameSceneNode = ParseOffset(clientJson, "m_pGameSceneNode");
  Offsets::m_modelState = ParseOffset(clientJson, "m_modelState");
  Offsets::m_hPlayerPawn = ParseOffset(clientJson, "m_hPlayerPawn");
  Offsets::m_iszPlayerName = ParseOffset(clientJson, "m_iszPlayerName");

  // Fallback if not found
  if (Offsets::dwEntityList == 0) {
    std::cerr << "[ERROR] Failed to parse dwEntityList!" << std::endl;
    return false;
  }

  std::cout << "[INFO] Successfully parsed offsets:" << std::endl;
  std::cout << "  - dwEntityList: 0x" << std::hex << Offsets::dwEntityList
            << std::endl;
  std::cout << "  - dwLocalPlayerPawn: 0x" << std::hex
            << Offsets::dwLocalPlayerPawn << std::endl;
  std::cout << "  - dwViewMatrix: 0x" << std::hex << Offsets::dwViewMatrix
            << std::endl;
  std::cout << "  - m_iHealth: 0x" << std::hex << Offsets::m_iHealth
            << std::endl;
  std::cout << "  - m_iTeamNum: 0x" << std::hex << Offsets::m_iTeamNum
            << std::endl;
  std::cout << "  - m_vOldOrigin: 0x" << std::hex << Offsets::m_vOldOrigin
            << std::endl;
  std::cout << "  - m_pGameSceneNode: 0x" << std::hex
            << Offsets::m_pGameSceneNode << std::endl;
  std::cout << "  - m_modelState: 0x" << std::hex << Offsets::m_modelState
            << std::endl;
  std::cout << "  - m_hPlayerPawn: 0x" << std::hex << Offsets::m_hPlayerPawn
            << std::dec << std::endl;

  return true;
}

} // namespace SDK
