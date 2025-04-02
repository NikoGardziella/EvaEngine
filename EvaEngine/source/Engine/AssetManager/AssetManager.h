#pragma once

#include <string>
#include <filesystem>
#include <mutex>

namespace Engine {

    class AssetManager
    {
    public:
        static void Initialize(int maxDepth = 5);

        static std::filesystem::path GetAssetPath(const std::string& subPath);
        static std::filesystem::path GetScenePath(const std::string& subPath);
        static std::filesystem::path GetAssetFolderPath();

        static std::filesystem::path GetCacheDirectory();
        static std::filesystem::path GetVulkanCacheDirectory();
        static void CreateCacheDirectoryIfNeeded();
        static std::vector<char> ReadFile(const std::string& filename);

    private:
        static std::filesystem::path s_AssetPath;
        static std::mutex s_Mutex;
    };

}
