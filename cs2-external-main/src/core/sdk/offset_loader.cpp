#include "offset_loader.h"
#include "offsets.h"
#include <atomic>
#include <iostream>
#include <nlohmann/json.hpp>
#include <windows.h>
#include <wininet.h>
#include <fstream>
#include <filesystem>

namespace SDK {

static std::filesystem::path GetExeDirectory() {
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
}

static std::string ReadFileFromExeDir(const std::string& relativePath) {
    auto exeDir = GetExeDirectory();
    auto fullPath = exeDir / relativePath;
    std::ifstream file(fullPath.string());
    if (!file.is_open()) return "";
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return content;
}

static std::string ReadFileFromProjectRoot(const std::string& relativePath) {
    auto exeDir = GetExeDirectory();
    // Try: exe dir, then parent (build/), then parent's parent (project root)
    std::filesystem::path paths[] = {
        exeDir / relativePath,
        exeDir.parent_path() / relativePath,
        exeDir.parent_path().parent_path() / relativePath
    };
    for (const auto& p : paths) {
        std::ifstream file(p.string());
        if (file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            return content;
        }
    }
    return "";
}

std::string OffsetLoader::FetchHTTP(const std::string& url, int timeoutSeconds) {
    HINTERNET hInt = InternetOpenA("Mozilla/5.0 (Windows NT 10.0; Win64; x64)", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hInt) return {};

    DWORD timeoutMs = timeoutSeconds * 1000;
    InternetSetOption(hInt, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeoutMs, sizeof(timeoutMs));
    InternetSetOption(hInt, INTERNET_OPTION_SEND_TIMEOUT, &timeoutMs, sizeof(timeoutMs));

    HINTERNET hUrl = InternetOpenUrlA(hInt, url.c_str(), nullptr, 0,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE, 0);
    if (!hUrl) {
        InternetCloseHandle(hInt);
        return {};
    }

    std::string result;
    char buf[8193];
    DWORD n = 0;
    DWORD totalRead = 0;
    const DWORD MAX_READ_SIZE = 1024 * 1024;

    while (InternetReadFile(hUrl, buf, sizeof(buf) - 1, &n) && n > 0) {
        totalRead += n;
        if (totalRead > MAX_READ_SIZE) break;
        buf[n] = '\0';
        result.append(buf, n);
    }

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInt);
    return result;
}

std::string OffsetLoader::ReadFileToString(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return content;
}

void OffsetLoader::WriteStringToFile(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::out | std::ios::trunc);
    if (file.is_open()) file << content;
}

ptrdiff_t OffsetLoader::ParseOffset(const std::string& src, const std::string& key) {
    try {
        auto json = nlohmann::json::parse(src);
        if (key.find('.') != std::string::npos) {
            auto parts = key;
            auto dotPos = parts.find('.');
            auto module = parts.substr(0, dotPos);
            auto offset = parts.substr(dotPos + 1);
            if (json.contains(module) && json[module].contains(offset)) {
                auto val = json[module][offset];
                if (val.is_string()) {
                    return static_cast<ptrdiff_t>(std::stoull(val.get<std::string>(), nullptr, 0));
                }
                return static_cast<ptrdiff_t>(val.get<uint64_t>());
            }
        } else {
            if (json.contains(key)) {
                auto val = json[key];
                if (val.is_string()) {
                    return static_cast<ptrdiff_t>(std::stoull(val.get<std::string>(), nullptr, 0));
                }
                return static_cast<ptrdiff_t>(val.get<uint64_t>());
            }
        }
    } catch (...) {
    }
    return 0;
}

static ptrdiff_t FindFieldInClientJson(const nlohmann::json& clientJson, const std::string& fieldName) {
    if (!clientJson.contains("client.dll") || !clientJson["client.dll"].contains("classes")) {
        return 0;
    }
    const auto& classes = clientJson["client.dll"]["classes"];
    for (auto it = classes.begin(); it != classes.end(); ++it) {
        if (it.value().contains("fields") && it.value()["fields"].contains(fieldName)) {
            auto val = it.value()["fields"][fieldName];
            if (val.is_string()) {
                return static_cast<ptrdiff_t>(std::stoull(val.get<std::string>(), nullptr, 0));
            }
            return static_cast<ptrdiff_t>(val.get<uint64_t>());
        }
    }
    return 0;
}

void OffsetLoader::ApplyOffsetsFromJson(const nlohmann::json& offsetsJson, const nlohmann::json& clientJson) {
    auto getVal = [](const nlohmann::json& j, const std::string& key) -> ptrdiff_t {
        if (j.contains(key)) {
            auto val = j[key];
            if (val.is_string()) {
                return static_cast<ptrdiff_t>(std::stoull(val.get<std::string>(), nullptr, 0));
            }
            return static_cast<ptrdiff_t>(val.get<uint64_t>());
        }
        return 0;
    };

    if (offsetsJson.contains("client.dll")) {
        const auto& client = offsetsJson["client.dll"];
        Offsets::dwEntityList = getVal(client, "dwEntityList");
        Offsets::dwLocalPlayerPawn = getVal(client, "dwLocalPlayerPawn");
        Offsets::dwLocalPlayerController = getVal(client, "dwLocalPlayerController");
        Offsets::dwViewMatrix = getVal(client, "dwViewMatrix");
        Offsets::dwPlantedC4 = getVal(client, "dwPlantedC4");
    }

    Offsets::m_fFlags = FindFieldInClientJson(clientJson, "m_fFlags");
    Offsets::m_vecVelocity = FindFieldInClientJson(clientJson, "m_vecVelocity");

    Offsets::m_iHealth = FindFieldInClientJson(clientJson, "m_iHealth");
    Offsets::m_iTeamNum = FindFieldInClientJson(clientJson, "m_iTeamNum");
    Offsets::m_vOldOrigin = FindFieldInClientJson(clientJson, "m_vOldOrigin");
    Offsets::m_pGameSceneNode = FindFieldInClientJson(clientJson, "m_pGameSceneNode");
    Offsets::m_modelState = FindFieldInClientJson(clientJson, "m_modelState");
    Offsets::m_hPlayerPawn = FindFieldInClientJson(clientJson, "m_hPlayerPawn");
    Offsets::m_iszPlayerName = FindFieldInClientJson(clientJson, "m_iszPlayerName");
    Offsets::m_pClippingWeapon = FindFieldInClientJson(clientJson, "m_pClippingWeapon");
    Offsets::m_vecViewOffset = FindFieldInClientJson(clientJson, "m_vecViewOffset");
    Offsets::m_angEyeAngles = FindFieldInClientJson(clientJson, "m_angEyeAngles");
    Offsets::m_aimPunchAngle = FindFieldInClientJson(clientJson, "m_aimPunchAngle");
    Offsets::m_iIDEntIndex = FindFieldInClientJson(clientJson, "m_iIDEntIndex");
    Offsets::m_bIsScoped = FindFieldInClientJson(clientJson, "m_bIsScoped");
    Offsets::m_iShotsFired = FindFieldInClientJson(clientJson, "m_iShotsFired");
    Offsets::m_nBombSite = FindFieldInClientJson(clientJson, "m_nBombSite");
    Offsets::m_bBombTicking = FindFieldInClientJson(clientJson, "m_bBombTicking");
    Offsets::m_flTimerLength = FindFieldInClientJson(clientJson, "m_flTimerLength");
}

void OffsetLoader::ApplyOffsets(const std::string& offsetsJson, const std::string& clientJson) {
    try {
        auto offsetsParsed = nlohmann::json::parse(offsetsJson);
        auto clientParsed = nlohmann::json::parse(clientJson);
        ApplyOffsetsFromJson(offsetsParsed, clientParsed);
    } catch (...) {
    }
}

void OffsetLoader::LoadFromCache() {
    std::string part1 = "https://raw.";
    std::string part2 = "github.";
    std::string part3 = "com/a2x/cs2-dumper/main/output/";
    const std::string BASE = part1 + part2 + part3;

    std::string offsetsJson = ReadFileToString(CACHE_FILE);
    std::string clientJson = ReadFileToString(CLIENT_CACHE_FILE);

    bool cacheValid = !offsetsJson.empty() && !clientJson.empty();
    if (cacheValid) {
        try {
            auto j = nlohmann::json::parse(offsetsJson);
            cacheValid = j.contains("dwEntityList") || j.contains("client_dll");
        } catch (...) { cacheValid = false; }
    }

    if (cacheValid) {
        ApplyOffsets(offsetsJson, clientJson);
        std::cout << "[+] Offsets loaded from cache.\n";
    } else {
        std::cout << "[+] Cache invalid, fetching offsets from GitHub...\n";
        std::string freshOffsets = FetchHTTP(BASE + "offsets.json");
        std::string freshClient = FetchHTTP(BASE + "client_dll.json");

        bool freshValid = !freshOffsets.empty() && !freshClient.empty();
        if (freshValid) {
            try {
                auto j = nlohmann::json::parse(freshOffsets);
                freshValid = j.contains("dwEntityList") || j.contains("client_dll");
            } catch (...) { freshValid = false; }
        }

        if (freshValid) {
            ApplyOffsets(freshOffsets, freshClient);
            WriteStringToFile(CACHE_FILE, freshOffsets);
            WriteStringToFile(CLIENT_CACHE_FILE, freshClient);
            std::cout << "[+] Offsets updated from GitHub!\n";
        } else {
            if (!offsetsJson.empty() && !clientJson.empty()) {
                ApplyOffsets(offsetsJson, clientJson);
                std::cout << "[!] Using stale cache (GitHub unavailable)\n";
            } else {
                // Fallback to bundled offsets.json
                std::string bundledOffsets = ReadFileFromProjectRoot("offsets.json");
                std::string bundledClient = ReadFileFromProjectRoot("data/client_dll.json");
                if (!bundledOffsets.empty() && !bundledClient.empty()) {
                    ApplyOffsets(bundledOffsets, bundledClient);
                    std::cout << "[+] Offsets loaded from bundled files.\n";
                } else {
                    std::cout << "[!] Failed to load offsets from any source!\n";
                }
            }
        }
    }
}

void OffsetLoader::ForceUpdateFromGitHub() {
    std::string part1 = "https://raw.";
    std::string part2 = "github.";
    std::string part3 = "com/a2x/cs2-dumper/main/output/";
    const std::string BASE = part1 + part2 + part3;

    std::cout << "[+] Fetching offsets from GitHub...\n";
    std::string offsetsJson = FetchHTTP(BASE + "offsets.json");
    std::string clientJson = FetchHTTP(BASE + "client_dll.json");

    if (offsetsJson.empty() || clientJson.empty()) {
        std::cout << "[!] Failed to fetch offsets from GitHub.\n";
        return;
    }

    bool valid = true;
    try {
        auto j = nlohmann::json::parse(offsetsJson);
        valid = j.contains("dwEntityList") || j.contains("client_dll");
    } catch (...) { valid = false; }

    if (!valid) {
        std::cout << "[!] Invalid JSON received from GitHub.\n";
        return;
    }

    ApplyOffsets(offsetsJson, clientJson);
    WriteStringToFile(CACHE_FILE, offsetsJson);
    WriteStringToFile(CLIENT_CACHE_FILE, clientJson);
    std::cout << "[+] Offsets updated from GitHub!\n";
}

std::future<bool> OffsetLoader::LoadOffsets() {
    return std::async(std::launch::async, [this]() {
        LoadFromCache();
        return true;
    });
}

std::future<bool> OffsetLoader::ForceUpdate() {
    return std::async(std::launch::async, [this]() {
        ForceUpdateFromGitHub();
        return true;
    });
}

} // namespace SDK
