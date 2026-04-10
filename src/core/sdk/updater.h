#pragma once
#include "offset_loader.h"
#include <future>

namespace SDK {

class Updater {
public:
    static std::future<bool> UpdateOffsets() {
        return std::async(std::launch::async, []() {
            try {
                static OffsetLoader loader;
                return loader.LoadOffsets();
            } catch (...) {
                return false;
            }
        });
    }

    static std::future<bool> ForceUpdateOffsets() {
        return std::async(std::launch::async, []() {
            try {
                static OffsetLoader loader;
                return loader.ForceUpdateFromGitHub();
            } catch (...) {
                return false;
            }
        });
    }

    static std::future<bool> ReloadOffsets() {
        return std::async(std::launch::async, []() {
            try {
                static OffsetLoader loader;
                return loader.ReloadOffsets();
            } catch (...) {
                return false;
            }
        });
    }
};

} // namespace SDK
