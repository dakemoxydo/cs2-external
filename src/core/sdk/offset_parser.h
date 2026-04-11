#pragma once
#include "offsets.h"
#include <string>
#include <nlohmann/json.hpp>
#include "offset_file_loader.h"

namespace SDK {

class OffsetParser {
public:
    OffsetSet Parse(const OffsetFileLoader::FileResult& files);

private:
    OffsetSet ParseJson(const std::string& offsetsJson, const std::string& clientJson);
    OffsetSet ParseHpp(const std::string& offsetsHpp, const std::string& clientHpp);
    ptrdiff_t GetVal(const nlohmann::json& j, const std::string& key);
    ptrdiff_t FindField(const nlohmann::json& clientJson, const std::string& fieldName);
};

} // namespace SDK
