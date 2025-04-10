#pragma once

#include <string>
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <Engine/Platform/Vulkan/VulkanTexture.h>

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

        static Ref<VulkanTexture> AddTexture(const std::string& name, const std::string& path);
		static Ref<VulkanTexture> GetTexture(const std::string& name);
        static  std::vector<Ref<VulkanTexture>> AssetManager::GetAllTextures();


    private:
        static std::filesystem::path s_AssetPath;
        static std::mutex s_Mutex;

        static std::unordered_map<std::string, std::shared_ptr<VulkanTexture>> s_textureCache;
    };

}
