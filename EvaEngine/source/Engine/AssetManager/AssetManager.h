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
        static std::filesystem::path GetAssetFolderPath();

        static std::filesystem::path GetCacheDirectory();
        static void CreateCacheDirectoryIfNeeded();

    private:
        static std::filesystem::path s_AssetPath;
        static std::mutex s_Mutex;
    };

}
