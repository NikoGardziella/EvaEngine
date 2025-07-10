#pragma once

#include <string>
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <Engine/Platform/Vulkan/VulkanTexture.h>
#include <Engine/Platform/Vulkan/Pixel/VulkanPixelTexture.h>

namespace Engine {

    class AssetManager
    {
    public:
        static void Initialize(int maxDepth = 5);

        static void CreateTileAtlas();

        static std::filesystem::path GetAssetPath(const std::string& subPath);
        static std::filesystem::path GetScenePath(const std::string& subPath);
        static std::filesystem::path GetAssetFolderPath();

        static std::filesystem::path GetCacheDirectory();
        static std::filesystem::path GetVulkanCacheDirectory();
        static void CreateCacheDirectoryIfNeeded();
        static std::vector<char> ReadFile(const std::string& filename);

        static Ref<VulkanTexture> AddTexture(const std::string& name, const std::string& path, bool imGuiTexture = false, uint32_t textureID = 0);
        static Ref<VulkanTexture> AddTextureToCache(const std::string& name, Ref<VulkanTexture> texture);
        static Ref<VulkanPixelTexture> AddPixelTexture(const std::string& name, const std::string& path);
		static Ref<VulkanTexture> GetTexture(const std::string& name);
        static Ref<VulkanTexture> CloneTexture(const std::string& name);
		static Ref<VulkanPixelTexture> GetPixelTexture(const std::string& name);
        static bool ExtractPixelsFromTilePallette(const glm::vec4& uv, std::vector<uint8_t>& outPixelData, int& outWidth, int& outHeight);
        static  std::vector<Ref<VulkanTexture>> AssetManager::GetAllTextures();

        static VkDeviceSize s_totalTextureMemory;

        // Texture streaming system
        static bool GetTexturePixelData(const std::string& textureName, std::vector<uint8_t>& outPixels, int& outWidth, int& outHeight);
        static std::string ResolveTexturePath(const std::string& textureName);
        
        //Tile atlas
		static std::unordered_map<std::string, glm::vec4>& GetTileUVMap() { return s_tileUVMap; }
		static std::vector<std::string>& GetTileNames() { return s_tileNames; }
		static Ref<VulkanTexture>& GetTileTextureIconAtlas() { return s_tileTextureIconAtlas; }
		

    private:
        static std::filesystem::path s_AssetPath;
        static std::mutex s_Mutex;

        static std::unordered_map<std::string, std::shared_ptr<VulkanTexture>> s_textureCache;
        static std::unordered_map<std::string, std::shared_ptr<VulkanPixelTexture>> s_pixelTextureCache;
        static Ref<VulkanTexture> s_tileTextureIconAtlas;

        static std::unordered_map<std::string, glm::vec4> s_tileUVMap;
        static std::vector<std::string> s_tileNames;


    };

}
