#pragma once
#include "offset_file_loader.h"
#include "offset_parser.h"
#include "offset_applier.h"

namespace SDK {

class OffsetLoader {
public:
    void LoadOffsets();
    void ReloadOffsets();
    void ForceUpdateFromGitHub();

private:
    OffsetFileLoader fileLoader;
    OffsetParser parser;
    OffsetApplier applier;
};

} // namespace SDK
