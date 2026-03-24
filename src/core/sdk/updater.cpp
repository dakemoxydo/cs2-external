#include "updater.h"
#include "offsets.h"
#include <atomic>
#include <iostream>
#include <regex>
#include <string>
#include <thread>
#include <windows.h>
#include <wininet.h>
#include <fstream>
#include <filesystem>

#pragma comment(lib, "wininet.lib")

namespace SDK {

// Cache file path for storing offsets locally
static const std::string CACHE_FILE = "offsets_cache.json";
static const std::string CLIENT_CACHE_FILE = "client_cache.json";

// ─── HTTP helper with timeout and error handling ──────────────────────────────
static std::string FetchHTTP(const std::string &url, int timeoutSeconds = 10) {
  HINTERNET hInt = InternetOpenA("CS2Overlay/1.0", INTERNET_OPEN_TYPE_PRECONFIG,
                                 nullptr, nullptr, 0);
  if (!hInt) {
    return {};
  }

  // Set receive timeout (in milliseconds)
  DWORD timeoutMs = timeoutSeconds * 1000;
  InternetSetOption(hInt, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeoutMs, sizeof(timeoutMs));
  InternetSetOption(hInt, INTERNET_OPTION_SEND_TIMEOUT, &timeoutMs, sizeof(timeoutMs));

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
  DWORD totalRead = 0;
  const DWORD MAX_READ_SIZE = 1024 * 1024; // 1MB max
  
  while (InternetReadFile(hUrl, buf, sizeof(buf) - 1, &n) && n > 0) {
    totalRead += n;
    if (totalRead > MAX_READ_SIZE) {
      break;
    }
    buf[n] = '\0';
    result.append(buf, n);
  }

  InternetCloseHandle(hUrl);
  InternetCloseHandle(hInt);
  
  return result;
}

// ─── Cache helper functions ───────────────────────────────────────────────────
static std::string ReadFileToString(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open())
    return "";
  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
  return content;
}

static void WriteStringToFile(const std::string &path, const std::string &content) {
  std::ofstream file(path, std::ios::out | std::ios::trunc);
  if (file.is_open()) {
    file << content;
  }
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
  Offsets::m_vecViewOffset = ParseOffset(clientJson, "m_vecViewOffset");


  // Aimbot / Triggerbot
  Offsets::m_angEyeAngles = ParseOffset(clientJson, "m_angEyeAngles");
  Offsets::m_aimPunchAngle = ParseOffset(clientJson, "m_aimPunchAngle");
  Offsets::m_iCrosshairEntityHandle =
      ParseOffset(clientJson, "m_iCrosshairEntityHandle");
  Offsets::m_bIsScoped = ParseOffset(clientJson, "m_bIsScoped");
  Offsets::m_iShotsFired = ParseOffset(clientJson, "m_iShotsFired");

  // Bomb
  Offsets::m_nBombSite = ParseOffset(clientJson, "m_nBombSite");
  Offsets::m_bBombTicking = ParseOffset(clientJson, "m_bBombTicking");
  Offsets::m_flTimerLength = ParseOffset(clientJson, "m_flTimerLength");

  // Note: If offsets from dumper are 0, they will remain 0
  // This indicates an issue with the dumper or CS2 update
  // Silent mode - no logging
}

// ─── Background fetch thread ─────────────────────────────────────────────────
static std::atomic<bool> s_fetchDone{false};

static void FetchThreadProc() {
  // Obfuscated GitHub URL - split to avoid static detection
  std::string part1 = "https://raw.";
  std::string part2 = "github.";
  std::string part3 = "com/a2x/cs2-dumper/main/output/";
  const std::string BASE = part1 + part2 + part3;
  
  // First try to load from local cache
  std::string offsetsJson = ReadFileToString(CACHE_FILE);
  std::string clientJson = ReadFileToString(CLIENT_CACHE_FILE);
  
  bool useCache = !offsetsJson.empty() && !clientJson.empty();
  
  // If cache is missing or invalid, try to fetch from network
  if (!useCache || ParseOffset(offsetsJson, "dwEntityList") == 0) {
    offsetsJson = FetchHTTP(BASE + "offsets.json");
    clientJson = FetchHTTP(BASE + "client_dll.json");

    if (offsetsJson.empty() || clientJson.empty()) {
      s_fetchDone = true;
      return;
    }

    if (ParseOffset(offsetsJson, "dwEntityList") == 0) {
      s_fetchDone = true;
      return;
    }
    
    // Save to cache for future use
    WriteStringToFile(CACHE_FILE, offsetsJson);
    WriteStringToFile(CLIENT_CACHE_FILE, clientJson);
  }

  ApplyOffsets(offsetsJson, clientJson);
  s_fetchDone = true;
}

// ─── Public: kick off async update, returns immediately ──────────────────────
bool Updater::UpdateOffsets() {
  std::thread(FetchThreadProc).detach();
  return true; // Main thread continues with default/last offsets immediately
}

} // namespace SDK
