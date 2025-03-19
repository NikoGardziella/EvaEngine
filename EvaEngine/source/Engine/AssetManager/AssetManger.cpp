#include "pch.h"
#include "AssetManger.h"

namespace Engine {



    std::string AssetManager::s_AssetPath = "";

    void AssetManager::Initialize(int maxDepth)
    {
        std::filesystem::path currentPath = std::filesystem::current_path();
        int depth = 0;

        while (!std::filesystem::exists(currentPath / "assets") && currentPath.has_parent_path() && depth < maxDepth)
        {
            currentPath = currentPath.parent_path(); // Move up one level
            depth++;
        }

        if (std::filesystem::exists(currentPath / "assets"))
        {
            s_AssetPath = (currentPath / "assets").string() + "/";
            EE_CORE_INFO("asset folder found at: {0} ", s_AssetPath);
        }
        else
        {
            EE_CORE_INFO("Coult not find asset folder at: {0} ", s_AssetPath);
        }
    }

    std::string AssetManager::GetAssetPath(const std::string& subPath)
    {
        return s_AssetPath + subPath;
    }

    std::string AssetManager::GetAssetFolderPath()
    {
        return s_AssetPath;
    }

    std::string AssetManager::GetCacheDirectory()
    {
        return GetAssetPath("cache/shader/opengl");
    }

    void AssetManager::CreateCacheDirectoryIfNeeded()
    {
        std::string cacheDirectory = GetCacheDirectory();
        if (!std::filesystem::exists(cacheDirectory))
        {
            std::filesystem::create_directories(cacheDirectory);
            std::cout << "Created cache directory: " << cacheDirectory << std::endl;
        }
    }



}