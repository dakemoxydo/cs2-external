#include "offset_loader.h"
#include <iostream>

namespace SDK {

namespace {

bool HasRequiredOffsets(const OffsetParser::ParsedOffsets& parsed) {
    return parsed.dwEntityList != 0
        && parsed.dwLocalPlayerPawn != 0
        && parsed.dwViewMatrix != 0;
}

} // namespace

void OffsetLoader::LoadOffsets() {
    auto files = fileLoader.LoadFromCacheDir();

    if (files.hasAny()) {
        auto parsed = parser.Parse(files);
        applier.Apply(parsed);
        applier.LogStatus();
        if (applier.Validate()) {
            std::cout << "[+] Offsets loaded from cache_offsets/.\n";
        } else {
            std::cout << "[!] Some offsets missing from cache, trying GitHub...\n";
            auto ghFiles = fileLoader.DownloadFromGitHub();
            if (!ghFiles.offsetsJson.empty() && !ghFiles.clientJson.empty()) {
                auto ghParsed = parser.Parse(ghFiles);
                if (HasRequiredOffsets(ghParsed)) {
                    fileLoader.SaveToCacheDir(ghFiles.offsetsJson, ghFiles.clientJson);
                    applier.Apply(ghParsed);
                    applier.LogStatus();
                    std::cout << "[+] Offsets updated from GitHub.\n";
                } else {
                    std::cout << "[!] GitHub returned incomplete or invalid offsets, keeping cache.\n";
                }
            } else {
                std::cout << "[!] Failed to fetch offsets from GitHub.\n";
            }
        }
    } else {
        auto ghFiles = fileLoader.DownloadFromGitHub();
        if (!ghFiles.offsetsJson.empty() && !ghFiles.clientJson.empty()) {
            auto parsed = parser.Parse(ghFiles);
            if (HasRequiredOffsets(parsed)) {
                fileLoader.SaveToCacheDir(ghFiles.offsetsJson, ghFiles.clientJson);
                applier.Apply(parsed);
                applier.LogStatus();
                std::cout << "[+] Offsets updated from GitHub.\n";
            } else {
                std::cout << "[!] Failed to load valid offsets from GitHub.\n";
            }
        } else {
            std::cout << "[!] Failed to load offsets from any source!\n";
        }
    }
}

void OffsetLoader::ReloadOffsets() {
    std::cout << "[+] Reloading offsets from cache_offsets/...\n";
    auto files = fileLoader.LoadFromCacheDir();
    if (files.hasAny()) {
        auto parsed = parser.Parse(files);
        applier.Apply(parsed);
        applier.LogStatus();
        if (applier.Validate()) {
            std::cout << "[+] Offsets reloaded successfully.\n";
        } else {
            std::cout << "[!] Warning: some offsets are missing after reload.\n";
        }
    } else {
        std::cout << "[!] No offset files found in cache_offsets/.\n";
    }
}

void OffsetLoader::ForceUpdateFromGitHub() {
    std::cout << "[+] Forcing offset update from GitHub...\n";
    auto ghFiles = fileLoader.DownloadFromGitHub();
    if (!ghFiles.offsetsJson.empty() && !ghFiles.clientJson.empty()) {
        auto parsed = parser.Parse(ghFiles);
        if (HasRequiredOffsets(parsed)) {
            fileLoader.SaveToCacheDir(ghFiles.offsetsJson, ghFiles.clientJson);
            applier.Apply(parsed);
            applier.LogStatus();
            std::cout << "[+] Offsets updated from GitHub!\n";
        } else {
            std::cout << "[!] Warning: GitHub returned incomplete or invalid offsets; cache not updated.\n";
        }
    } else {
        std::cout << "[!] Failed to fetch offsets from GitHub.\n";
    }
}

} // namespace SDK
