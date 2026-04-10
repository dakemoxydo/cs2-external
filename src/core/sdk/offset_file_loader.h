#pragma once
#include <string>

namespace SDK {

class OffsetFileLoader {
public:
    struct FileResult {
        std::string offsetsJson;
        std::string clientJson;
        std::string offsetsHpp;
        std::string clientHpp;

        bool hasJson() const { return !offsetsJson.empty() && !clientJson.empty(); }
        bool hasHpp() const { return !offsetsHpp.empty() || !clientHpp.empty(); }
        bool hasAny() const { return hasJson() || hasHpp(); }
    };

    FileResult LoadFromCacheDir();
    FileResult DownloadFromGitHub();
    void SaveToCacheDir(const std::string& offsetsJson, const std::string& clientJson);

private:
    std::string ReadFile(const std::string& path);
    void WriteFile(const std::string& path, const std::string& content);
    std::string FetchHTTP(const std::string& url, int timeoutSeconds = 10);
    std::string GetExeDir();
    std::string GetCacheDir();
};

} // namespace SDK
