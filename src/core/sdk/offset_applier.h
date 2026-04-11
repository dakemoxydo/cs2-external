#pragma once
#include "offset_parser.h"

namespace SDK {

class OffsetApplier {
public:
    void Apply(const OffsetSet& parsed);
    bool Validate(const OffsetSet& parsed) const;
    void LogStatus(const OffsetSet& parsed) const;
};

} // namespace SDK
