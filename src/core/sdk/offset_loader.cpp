#include "offset_loader.h"
#include <iostream>

namespace SDK {

namespace {

bool HasRequiredOffsets(const OffsetSet& parsed) {
    return parsed.HasRequired();
}

} // namespace

bool OffsetLoader::LoadOffsets() {
    auto files = fileLoader.LoadFromCacheDir();

    if (files.hasAny()) {
        auto parsed = parser.Parse(files);
        applier.Apply(parsed);
        applier.LogStatus(parsed);
        if (applier.Validate(parsed)) {
            std::cout << "[+] Offsets loaded from cache_offsets/.\n";
            return true;
        }

        std::cout << "[!] Some offsets missing from cache, trying GitHub...\n";
        auto ghFiles = fileLoader.DownloadFromGitHub();
        if (!ghFiles.offsetsJson.empty() && !ghFiles.clientJson.empty()) {
            auto ghParsed = parser.Parse(ghFiles);
            if (HasRequiredOffsets(ghParsed)) {
                fileLoader.SaveToCacheDir(ghFiles.offsetsJson, ghFiles.clientJson);
                applier.Apply(ghParsed);
                applier.LogStatus(ghParsed);
                if (applier.Validate(ghParsed)) {
                    std::cout << "[+] Offsets updated from GitHub.\n";
                    return true;
                }
                std::cout << "[!] GitHub offsets failed validation, keeping cache.\n";
                return false;
            }

            std::cout << "[!] GitHub returned incomplete or invalid offsets, keeping cache.\n";
            return false;
        }

        std::cout << "[!] Failed to fetch offsets from GitHub.\n";
        return false;
    }

    auto ghFiles = fileLoader.DownloadFromGitHub();
    if (!ghFiles.offsetsJson.empty() && !ghFiles.clientJson.empty()) {
        auto parsed = parser.Parse(ghFiles);
        if (HasRequiredOffsets(parsed)) {
            fileLoader.SaveToCacheDir(ghFiles.offsetsJson, ghFiles.clientJson);
            applier.Apply(parsed);
            applier.LogStatus(parsed);
            if (applier.Validate(parsed)) {
                std::cout << "[+] Offsets updated from GitHub.\n";
                return true;
            }
            std::cout << "[!] GitHub offsets failed validation.\n";
            return false;
        }

        std::cout << "[!] Failed to load valid offsets from GitHub.\n";
        return false;
    }

    std::cout << "[!] Failed to load offsets from any source!\n";
    return false;
}

bool OffsetLoader::ReloadOffsets() {
    std::cout << "[+] Reloading offsets from cache_offsets/...\n";
    auto files = fileLoader.LoadFromCacheDir();
    if (files.hasAny()) {
        auto parsed = parser.Parse(files);
        applier.Apply(parsed);
        applier.LogStatus(parsed);
        if (applier.Validate(parsed)) {
            std::cout << "[+] Offsets reloaded successfully.\n";
            return true;
        } else {
            std::cout << "[!] Warning: some offsets are missing after reload.\n";
            return false;
        }
    }

    std::cout << "[!] No offset files found in cache_offsets/.\n";
    return false;
}

bool OffsetLoader::ForceUpdateFromGitHub() {
    std::cout << "[+] Forcing offset update from GitHub...\n";
    auto ghFiles = fileLoader.DownloadFromGitHub();
    if (!ghFiles.offsetsJson.empty() && !ghFiles.clientJson.empty()) {
        auto parsed = parser.Parse(ghFiles);
        if (HasRequiredOffsets(parsed)) {
            fileLoader.SaveToCacheDir(ghFiles.offsetsJson, ghFiles.clientJson);
            applier.Apply(parsed);
            applier.LogStatus(parsed);
            std::cout << "[+] Offsets updated from GitHub!\n";
            return true;
        }

        std::cout << "[!] Warning: GitHub returned incomplete or invalid offsets; cache not updated.\n";
        return false;
    }

    std::cout << "[!] Failed to fetch offsets from GitHub.\n";
    return false;
}

} // namespace SDK
