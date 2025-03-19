#pragma once
#pragma onc
#include "Engine/Core/Core.h"

namespace Engine {



    class AssetManager
    {
    public:
        static void Initialize(int maxDepth = 5);

        static std::string GetAssetPath(const std::string& subPath);
        static std::string GetAssetFolderPath();
        static std::string GetCacheDirectory();

        static void CreateCacheDirectoryIfNeeded();

    private:
        static std::string s_AssetPath;
    };




}