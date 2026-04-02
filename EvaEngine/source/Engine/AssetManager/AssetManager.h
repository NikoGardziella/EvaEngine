#pragma once

#include <string>
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <Engine/Platform/Vulkan/VulkanTexture.h>
#include <Engine/Scene/Components/Render/TileComponent.h>
#include "Utils/TileSerializer.h"
#include <Engine/Animation/3D/MaterialRegistry.h>
#include <Engine/Animation/3D/Import/GLTFImporter.h>
#include <Engine/Animation/3D/MeshRegistry.h>
#include "Utils/AssetManagerUtils.h"
#include <Engine/Core/Core.h>
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <Engine/Animation/3D/AnimationRegistry.h>
#include "Font/FontLoader.h"
#include <Engine/UI/Font.h>


#include "glm/glm.hpp"
#include <Engine/Animation/3D/SkeletonRegistry.h>

namespace Engine {

   

    class AssetManager
    {
        

    public:
        static void Initialize(int maxDepth = 5);

        static void CreateTileAtlas();

        static eTileMaterial ParseMaterialFromPath(const std::filesystem::path& path);


        static std::filesystem::path GetAssetPath(const std::string& subPath);
        static std::filesystem::path GetScenePath(const std::string& subPath);
        static std::filesystem::path GetAssetFolderPath();

        static std::filesystem::path GetCacheDirectory();
        static std::filesystem::path GetVulkanCacheDirectory();
        static void CreateCacheDirectoryIfNeeded();
        static std::vector<char> ReadFile(const std::string& filename);

        static Ref<VulkanTexture> AddTexture(const std::string& name, const std::string& path, bool imGuiTexture = false, uint32_t textureID = 0);
        static Ref<VulkanTexture> AddTextureToCache(const std::string& name, Ref<VulkanTexture> texture);
		static Ref<VulkanTexture> GetTexture(const std::string& name);
        static Ref<VulkanTexture> CloneTexture(const std::string& name);
        static bool ExtractPixelsFromTilePallette(const TileInfo& tile, std::vector<uint8_t>& outPixelData, int& outWidth, int& outHeight);
        static bool ExtractPixelsAndPropertiesFromTilePallette(const TileInfo& tile, std::vector<uint8_t>& outPixelData, std::vector<uint8_t>& outPropertiesData, int& outWidth, int& outHeight, glm::ivec2& outOpaqueMin, glm::ivec2& outOpaqueMax);
        static bool ExtractPixelsFromTilePallette(const TileInfo& tile, std::vector<uint8_t>& outPixelData, int& outWidth, int& outHeight, glm::ivec2& outOpaqueMin, glm::ivec2& outOpaqueMax);
        static  std::vector<Ref<VulkanTexture>> AssetManager::GetAllTextures();

        static const std::vector<std::string>& AssetManager::GetTileNamesByCategoryAndMaterial(eTileCategory category, eTileMaterial material);
        static int AssetManager::GetTileCountForCategory(eTileCategory category);

        static Ref<VulkanTexture> GetTileTextureIconAtlas() { return s_tileTextureIconAtlas; };

        static const TileProperties& AssetManager::GetTileProperties(const std::string& tileName);

        //static void LoadTileProperties();

        static uint8_t PackCategoryNibble(eTileCategory c);

        static uint32_t ImportGLTF(const std::string& path);

        static GLTFImportOptions MakeDefaultGLTFOpts(AssetManagerUtils::GLTFAggregator& agg, bool flipV, bool genFlatNormalsIfMissing);

        static MeshAsset& GetMeshFromMeshRegistry(uint32_t meshId) { return s_meshRegistry.GetMesh(meshId); }
        static MeshRegistry& GetMeshRegistry() { return s_meshRegistry; }
        static MaterialRegistry& GetMaterialRegistry() { return s_materialRegistry; }
        static SkeletonRegistry& GetSkeletonRegistry()  { return *s_skeletonRegistry; }
        static AnimationRegistry& GetAnimationRegistry() { return *s_animationRegistry; }

        static const Ref<Font> GetFont() { return s_fontAtlas;  }

		static const std::unordered_map<std::string, glm::vec4>& AssetManager::GetTileTextureAtalsUVs() { return  s_tileUVMap; }

        // Texture streaming system
        static bool GetTexturePixelData(const std::string& textureName, std::vector<uint8_t>& outPixels, std::vector<uint8_t>& outHealthData, int& outWidth, int& outHeight);
        static std::string ResolveTexturePath(const std::string& textureName);
        
        //Tile atlas
        static const std::unordered_map<std::string, glm::vec4>& AssetManager::GetTileUVMap(const eTileCategory& category)
        {
            return s_tileUVMapsByCategory.at(category);
        }

        static const std::vector<std::string>& AssetManager::GetTileNames(const eTileCategory& category)
        {
            return s_tileNamesByCategory.at(category);
        }

        static Ref<VulkanTexture> AssetManager::GetTileAtlas(eTileCategory category)
        {
            return s_tileAtlasesByCategory.at(category);
        }


        static const std::unordered_map<eTileCategory, std::unordered_map<std::string, glm::vec4>>&
            AssetManager::GetAllTileUVMaps()
        {
            return s_tileUVMapsByCategory;
        }

        static const std::vector<std::string>& AssetManager::GetTileNamesByCategory(eTileCategory category);

    private:
        static std::filesystem::path s_AssetPath;
        static std::mutex s_Mutex;

        static std::unordered_map<std::string, std::shared_ptr<VulkanTexture>> s_textureCache;
       
        static std::unordered_map<eTileCategory, std::unordered_map<std::string, glm::vec4>> s_tileUVMapsByCategory;
        static std::unordered_map<eTileCategory, std::vector<std::string>> s_tileNamesByCategory;
        static std::unordered_map<std::string, glm::vec4> s_tileUVMap;
        static std::unordered_map<eTileCategory, Ref<VulkanTexture>> s_tileAtlasesByCategory;
        static std::unordered_map<eTileCategory, std::unordered_map<eTileMaterial, std::vector<std::string>>> s_tileNamesByCategoryAndMaterial;

		static Ref<VulkanTexture> s_tileTextureIconAtlas;
        static inline std::unordered_map<std::string, TileProperties> s_tileProperties;

        static MeshRegistry s_meshRegistry;
        static MaterialRegistry s_materialRegistry;
        static Ref<AnimationRegistry> s_animationRegistry;
        static Ref<SkeletonRegistry> s_skeletonRegistry;


        static Ref<Font> s_fontAtlas;

    };

}
