#pragma once
#include <string>
#include <future>
#include <nlohmann/json.hpp>

namespace SDK {

class OffsetLoader {
public:
    std::future<bool> LoadOffsets();
    std::future<bool> ForceUpdate();

private:
    std::string FetchHTTP(const std::string& url, int timeoutSeconds = 10);
    std::string ReadFileToString(const std::string& path);
    void WriteStringToFile(const std::string& path, const std::string& content);
    ptrdiff_t ParseOffset(const std::string& src, const std::string& key);
    void ApplyOffsetsFromJson(const nlohmann::json& offsetsJson, const nlohmann::json& clientJson);
    void ApplyOffsets(const std::string& offsetsJson, const std::string& clientJson);
    
    void LoadFromCache();
    void ForceUpdateFromGitHub();

    static inline const std::string CACHE_FILE = "offsets_cache.json";
    static inline const std::string CLIENT_CACHE_FILE = "client_cache.json";
};

} // namespace SDK
