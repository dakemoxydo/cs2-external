#include "offset_file_loader.h"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <windows.h>
#include <wininet.h>

namespace SDK {

std::string OffsetFileLoader::GetExeDir() {
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    return std::filesystem::path(path).parent_path().string();
}

std::string OffsetFileLoader::GetCacheDir() {
    return GetExeDir() + "/cache_offsets";
}

std::string OffsetFileLoader::ReadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return "";
    const auto end = file.tellg();
    constexpr std::streamoff kMaxOffsetFileSize = 32 * 1024 * 1024;
    if (end < 0 || end > kMaxOffsetFileSize) return "";
    file.seekg(0, std::ios::beg);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (file.bad()) return "";
    return content;
}

bool OffsetFileLoader::WriteFile(const std::string& path, const std::string& content) {
    auto parent = std::filesystem::path(path).parent_path();
    std::error_code ec;
    std::filesystem::create_directories(parent, ec);
    if (ec) return false;

    const std::string temporaryPath = path + ".tmp";
    std::ofstream file(temporaryPath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) return false;
    file.write(content.data(), static_cast<std::streamsize>(content.size()));
    file.flush();
    if (!file.good()) {
        file.close();
        std::filesystem::remove(temporaryPath, ec);
        return false;
    }
    file.close();

    if (!MoveFileExA(temporaryPath.c_str(), path.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        std::filesystem::remove(temporaryPath, ec);
        return false;
    }
    return true;
}

std::string OffsetFileLoader::FetchHTTP(const std::string& url, int timeoutSeconds) {
    constexpr int kAttempts = 3;
    constexpr size_t kMaxReadSize = 32u * 1024u * 1024u;

    for (int attempt = 0; attempt < kAttempts; ++attempt) {
        HINTERNET hInt = InternetOpenA(
            "cs2overlay-offset-updater/1.0", INTERNET_OPEN_TYPE_PRECONFIG,
            nullptr, nullptr, 0);
        if (!hInt) return {};

        DWORD timeoutMs = static_cast<DWORD>(timeoutSeconds * 1000);
        InternetSetOptionA(hInt, INTERNET_OPTION_CONNECT_TIMEOUT, &timeoutMs,
                           sizeof(timeoutMs));
        InternetSetOptionA(hInt, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeoutMs,
                           sizeof(timeoutMs));
        InternetSetOptionA(hInt, INTERNET_OPTION_SEND_TIMEOUT, &timeoutMs,
                           sizeof(timeoutMs));

        const DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE |
                            INTERNET_FLAG_NO_CACHE_WRITE |
                            INTERNET_FLAG_PRAGMA_NOCACHE |
                            INTERNET_FLAG_NO_UI;
        HINTERNET hUrl = InternetOpenUrlA(hInt, url.c_str(), nullptr, 0,
                                          flags, 0);
        DWORD statusCode = 0;
        if (hUrl) {
            DWORD statusSize = sizeof(statusCode);
            HttpQueryInfoA(hUrl,
                           HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                           &statusCode, &statusSize, nullptr);
        }

        std::string result;
        bool readSucceeded = hUrl && statusCode == HTTP_STATUS_OK;
        char buffer[16 * 1024];
        while (readSucceeded) {
            DWORD bytesRead = 0;
            if (!InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead)) {
                readSucceeded = false;
                break;
            }
            if (bytesRead == 0) break;
            if (result.size() + bytesRead > kMaxReadSize) {
                readSucceeded = false;
                break;
            }
            result.append(buffer, bytesRead);
        }

        if (hUrl) InternetCloseHandle(hUrl);
        InternetCloseHandle(hInt);
        if (readSucceeded && !result.empty()) return result;

        // Retry transient network errors and GitHub throttling. Permanent 4xx
        // responses are left for the caller's fallback endpoint.
        if (statusCode >= 400 && statusCode < 500 &&
            statusCode != HTTP_STATUS_REQUEST_TIMEOUT && statusCode != 429) {
            break;
        }
        if (attempt + 1 < kAttempts) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(250 * (attempt + 1)));
        }
    }
    return {};
}

OffsetFileLoader::FileResult OffsetFileLoader::LoadFromCacheDir() {
    FileResult result;
    std::string cacheDir = GetCacheDir();

    // The active marker is written only after a complete pair is persisted.
    std::string generation = ReadFile(cacheDir + "/active.txt");
    generation.erase(std::remove_if(generation.begin(), generation.end(),
                                    [](unsigned char ch) { return std::isspace(ch); }),
                     generation.end());
    const bool validGeneration = !generation.empty() &&
        std::all_of(generation.begin(), generation.end(), [](unsigned char ch) {
            return std::isalnum(ch) || ch == '-' || ch == '_';
        });
    if (validGeneration) {
        const std::string generationDir = cacheDir + "/" + generation;
        result.offsetsJson = ReadFile(generationDir + "/offsets.json");
        result.clientJson = ReadFile(generationDir + "/client_dll.json");
        if (result.hasJson()) return result;
        result = {};
    }

    // Compatibility with offsets copied manually by build.bat.
    result.offsetsJson = ReadFile(cacheDir + "/offsets.json");
    result.clientJson = ReadFile(cacheDir + "/client_dll.json");
    result.offsetsHpp = ReadFile(cacheDir + "/offsets.hpp");
    result.clientHpp = ReadFile(cacheDir + "/client_dll.hpp");

    return result;
}

OffsetFileLoader::FileResult OffsetFileLoader::DownloadFromGitHub() {
    std::cout << "[+] Fetching offsets from GitHub...\n";
    const std::string base =
        "https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/";
    FileResult result;
    result.offsetsJson = FetchHTTP(base + "offsets.json");
    result.clientJson = FetchHTTP(base + "client_dll.json");
    if (result.hasJson()) return result;

    // jsDelivr mirrors GitHub and is useful when raw.githubusercontent.com is
    // unavailable due to DNS filtering or a regional network problem.
    FileResult fallback;
    const std::string fallbackBase =
        "https://cdn.jsdelivr.net/gh/a2x/cs2-dumper@main/output/";
    fallback.offsetsJson = FetchHTTP(fallbackBase + "offsets.json");
    fallback.clientJson = FetchHTTP(fallbackBase + "client_dll.json");
    return fallback;
}

bool OffsetFileLoader::SaveToCacheDir(const std::string& offsetsJson, const std::string& clientJson) {
    if (offsetsJson.empty() || clientJson.empty()) return false;
    std::string cacheDir = GetCacheDir();
    const std::string generation = "generation-" + std::to_string(GetTickCount64());
    const std::string generationDir = cacheDir + "/" + generation;
    if (!WriteFile(generationDir + "/offsets.json", offsetsJson) ||
        !WriteFile(generationDir + "/client_dll.json", clientJson)) {
        std::error_code ec;
        std::filesystem::remove_all(generationDir, ec);
        return false;
    }

    // Replacing one small marker is the atomic commit point for the pair.
    return WriteFile(cacheDir + "/active.txt", generation + "\n");
}

} // namespace SDK
