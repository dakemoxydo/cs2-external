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
        applier.LogStatus(parsed);
        if (applier.Validate(parsed)) {
            applier.Apply(parsed);
            std::cout << "[+] Offsets loaded from cache_offsets/.\n";
            return true;
        }

        std::cout << "[!] Some offsets missing from cache, trying GitHub...\n";
        auto ghFiles = fileLoader.DownloadFromGitHub();
        if (!ghFiles.offsetsJson.empty() && !ghFiles.clientJson.empty()) {
            auto ghParsed = parser.Parse(ghFiles);
            applier.LogStatus(ghParsed);
            if (HasRequiredOffsets(ghParsed) && applier.Validate(ghParsed)) {
                if (!fileLoader.SaveToCacheDir(ghFiles.offsetsJson,
                                               ghFiles.clientJson)) {
                    std::cout << "[!] Offsets are valid but the cache could not be saved.\n";
                }
                applier.Apply(ghParsed);
                std::cout << "[+] Offsets updated from GitHub.\n";
                return true;
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
        applier.LogStatus(parsed);
        if (HasRequiredOffsets(parsed) && applier.Validate(parsed)) {
            if (!fileLoader.SaveToCacheDir(ghFiles.offsetsJson,
                                           ghFiles.clientJson)) {
                std::cout << "[!] Offsets are valid but the cache could not be saved.\n";
            }
            applier.Apply(parsed);
            std::cout << "[+] Offsets updated from GitHub.\n";
            return true;
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
        applier.LogStatus(parsed);
        if (applier.Validate(parsed)) {
            applier.Apply(parsed);
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
        if (HasRequiredOffsets(parsed) && applier.Validate(parsed)) {
            if (!fileLoader.SaveToCacheDir(ghFiles.offsetsJson,
                                           ghFiles.clientJson)) {
                std::cout << "[!] Offset cache could not be saved; keeping active offsets unchanged.\n";
                return false;
            }
            // Publish only after the complete cache generation has committed.
            // Readers receive this immutable snapshot atomically.
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
