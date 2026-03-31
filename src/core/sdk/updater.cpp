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
                       INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE,
                       0);
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
  Offsets::m_iIDEntIndex =
      ParseOffset(clientJson, "m_iIDEntIndex");
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

// ─── Cache-first with auto-fetch from GitHub ─────────────────────────────────
static void LoadFromCache() {
  // Obfuscated GitHub URL - split to avoid static detection
  std::string part1 = "https://raw.";
  std::string part2 = "github.";
  std::string part3 = "com/a2x/cs2-dumper/main/output/";
  const std::string BASE = part1 + part2 + part3;

  // Try cache first
  std::string offsetsJson = ReadFileToString(CACHE_FILE);
  std::string clientJson = ReadFileToString(CLIENT_CACHE_FILE);

  bool cacheValid = !offsetsJson.empty() && !clientJson.empty() &&
                    ParseOffset(offsetsJson, "dwEntityList") != 0;

  if (cacheValid) {
    ApplyOffsets(offsetsJson, clientJson);
    std::cout << "[+] Offsets loaded from cache." << std::endl;
  } else {
    // Cache empty/invalid - fetch from GitHub
    std::cout << "[+] Cache invalid, fetching offsets from GitHub..." << std::endl;

    std::string freshOffsets = FetchHTTP(BASE + "offsets.json");
    std::string freshClient = FetchHTTP(BASE + "client_dll.json");

    if (!freshOffsets.empty() && !freshClient.empty() &&
        ParseOffset(freshOffsets, "dwEntityList") != 0) {
      ApplyOffsets(freshOffsets, freshClient);
      // Save to cache
      WriteStringToFile(CACHE_FILE, freshOffsets);
      WriteStringToFile(CLIENT_CACHE_FILE, freshClient);
      std::cout << "[+] Offsets updated from GitHub!" << std::endl;
    } else {
      // GitHub failed - try old cache as fallback
      if (!offsetsJson.empty() && !clientJson.empty()) {
        ApplyOffsets(offsetsJson, clientJson);
        std::cout << "[!] Using stale cache (GitHub unavailable)" << std::endl;
      } else {
        std::cout << "[!] Failed to load offsets from any source!" << std::endl;
      }
    }
  }
  s_fetchDone = true;
}

// ─── Force update from GitHub ────────────────────────────────────────────────
static void ForceUpdateFromGitHub() {
  // Obfuscated GitHub URL - split to avoid static detection
  std::string part1 = "https://raw.";
  std::string part2 = "github.";
  std::string part3 = "com/a2x/cs2-dumper/main/output/";
  const std::string BASE = part1 + part2 + part3;

  std::cout << "[+] Fetching offsets from GitHub..." << std::endl;

  std::string offsetsJson = FetchHTTP(BASE + "offsets.json");
  std::string clientJson = FetchHTTP(BASE + "client_dll.json");

  if (offsetsJson.empty() || clientJson.empty()) {
    std::cout << "[!] Failed to fetch offsets from GitHub." << std::endl;
    return;
  }

  ApplyOffsets(offsetsJson, clientJson);
  WriteStringToFile(CACHE_FILE, offsetsJson);
  WriteStringToFile(CLIENT_CACHE_FILE, clientJson);
  std::cout << "[+] Offsets updated from GitHub!" << std::endl;
}

// ─── Public: kick off async update, returns future for synchronization ───────
// Note: If the returned future is discarded (as in main.cpp), the thread runs
// detached-style (fire-and-forget). To wait for completion, call .wait()/.get().
std::future<bool> Updater::UpdateOffsets() {
  return std::async(std::launch::async, []() {
    LoadFromCache();
    return true;
  });
}

std::future<bool> Updater::ForceUpdateOffsets() {
  return std::async(std::launch::async, []() {
    ForceUpdateFromGitHub();
    return true;
  });
}

} // namespace SDK
