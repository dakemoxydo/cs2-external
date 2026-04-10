#pragma once
#include "offset_parser.h"

namespace SDK {

class OffsetApplier {
public:
    void Apply(const OffsetParser::ParsedOffsets& parsed);
    bool Validate() const;
    void LogStatus() const;
};

} // namespace SDK
