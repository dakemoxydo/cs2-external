#include "offset_file_loader.h"
#include <iostream>
#include <fstream>
#include <filesystem>
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
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return "";
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return content;
}

void OffsetFileLoader::WriteFile(const std::string& path, const std::string& content) {
    auto parent = std::filesystem::path(path).parent_path();
    std::error_code ec;
    std::filesystem::create_directories(parent, ec);
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (file.is_open()) file << content;
}

std::string OffsetFileLoader::FetchHTTP(const std::string& url, int timeoutSeconds) {
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

OffsetFileLoader::FileResult OffsetFileLoader::LoadFromCacheDir() {
    FileResult result;
    std::string cacheDir = GetCacheDir();

    result.offsetsJson = ReadFile(cacheDir + "/offsets.json");
    result.clientJson = ReadFile(cacheDir + "/client_dll.json");
    result.offsetsHpp = ReadFile(cacheDir + "/offsets.hpp");
    result.clientHpp = ReadFile(cacheDir + "/client_dll.hpp");

    return result;
}

OffsetFileLoader::FileResult OffsetFileLoader::DownloadFromGitHub() {
    FileResult result;
    std::string part1 = "https://raw.";
    std::string part2 = "github.";
    std::string part3 = "com/a2x/cs2-dumper/main/output/";
    const std::string BASE = part1 + part2 + part3;

    std::cout << "[+] Fetching offsets from GitHub...\n";
    result.offsetsJson = FetchHTTP(BASE + "offsets.json");
    result.clientJson = FetchHTTP(BASE + "client_dll.json");

    return result;
}

void OffsetFileLoader::SaveToCacheDir(const std::string& offsetsJson, const std::string& clientJson) {
    std::string cacheDir = GetCacheDir();
    WriteFile(cacheDir + "/offsets.json", offsetsJson);
    WriteFile(cacheDir + "/client_dll.json", clientJson);
}

} // namespace SDK
