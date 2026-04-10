#pragma once
#include "offset_loader.h"
#include <future>

namespace SDK {

// Backward compatibility — делегируем OffsetLoader
class Updater {
public:
    static std::future<bool> UpdateOffsets() {
        static OffsetLoader loader;
        return loader.LoadOffsets();
    }

    static std::future<bool> ForceUpdateOffsets() {
        static OffsetLoader loader;
        return loader.ForceUpdate();
    }
};

} // namespace SDK
