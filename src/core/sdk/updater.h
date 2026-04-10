#pragma once
#include "offset_loader.h"
#include <future>

namespace SDK {

class Updater {
public:
    static std::future<bool> UpdateOffsets() {
        return std::async(std::launch::async, []() {
            static OffsetLoader loader;
            loader.LoadOffsets();
            return true;
        });
    }

    static std::future<bool> ForceUpdateOffsets() {
        return std::async(std::launch::async, []() {
            static OffsetLoader loader;
            loader.ForceUpdateFromGitHub();
            return true;
        });
    }

    static std::future<bool> ReloadOffsets() {
        return std::async(std::launch::async, []() {
            static OffsetLoader loader;
            loader.ReloadOffsets();
            return true;
        });
    }
};

} // namespace SDK
