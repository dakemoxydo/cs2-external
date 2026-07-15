#include "offset_file_loader.h"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <filesystem>
#include <regex>
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
    HINTERNET hInt = InternetOpenA("Mozilla/5.0 (Windows NT 10.0; Win64; x64)", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hInt) return {};

    DWORD timeoutMs = timeoutSeconds * 1000;
    InternetSetOption(hInt, INTERNET_OPTION_CONNECT_TIMEOUT, &timeoutMs, sizeof(timeoutMs));
    InternetSetOption(hInt, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeoutMs, sizeof(timeoutMs));
    InternetSetOption(hInt, INTERNET_OPTION_SEND_TIMEOUT, &timeoutMs, sizeof(timeoutMs));

    HINTERNET hUrl = InternetOpenUrlA(hInt, url.c_str(), nullptr, 0,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE, 0);
    if (!hUrl) {
        InternetCloseHandle(hInt);
        return {};
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    if (!HttpQueryInfoA(hUrl, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                        &statusCode, &statusSize, nullptr) || statusCode != 200) {
        InternetCloseHandle(hUrl);
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
        if (totalRead > MAX_READ_SIZE) {
            result.clear();
            break;
        }
        buf[n] = '\0';
        result.append(buf, n);
    }

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInt);
    return result;
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
    FileResult result;

    std::cout << "[+] Fetching offsets from GitHub...\n";
    const std::string commitInfo = FetchHTTP(
        "https://api.github.com/repos/a2x/cs2-dumper/commits/main");
    std::smatch match;
    static const std::regex shaPattern(
        R"sha("sha"\s*:\s*"([0-9a-fA-F]{40})")sha");
    if (!std::regex_search(commitInfo, match, shaPattern)) {
        std::cout << "[!] Could not resolve a cs2-dumper commit.\n";
        return result;
    }
    const std::string base = "https://raw.githubusercontent.com/a2x/cs2-dumper/" +
                             match[1].str() + "/output/";
    result.offsetsJson = FetchHTTP(base + "offsets.json");
    result.clientJson = FetchHTTP(base + "client_dll.json");

    return result;
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
