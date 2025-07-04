#include "pch.h"
#include "AssetManager.h"
#include <iostream>
#include <mutex>
#include <Engine/Platform/Vulkan/Pixel/VulkanPixelTexture.h>
#include <stb_image.h>

namespace Engine {

    std::filesystem::path AssetManager::s_AssetPath = "";

    // prevent multiple threads from accessing shared resources simultaneously
    std::mutex AssetManager::s_Mutex;
    std::unordered_map<std::string, std::shared_ptr<VulkanTexture>> AssetManager::s_textureCache;
    std::unordered_map<std::string, std::shared_ptr<VulkanPixelTexture>> AssetManager::s_pixelTextureCache;
    VkDeviceSize AssetManager::s_totalTextureMemory;



    void AssetManager::Initialize(int maxDepth)
    {

        //stbi_set_flip_vertically_on_load(true);
        std::lock_guard<std::mutex> lock(s_Mutex); // Ensure thread safety

        std::filesystem::path currentPath = std::filesystem::current_path();
        int depth = 0;

        while (!std::filesystem::exists(currentPath / "assets") && currentPath.has_parent_path() && depth < maxDepth)
        {
            currentPath = currentPath.parent_path(); // Move up one level
            depth++;
        }

        if (std::filesystem::exists(currentPath / "assets"))
        {
            s_AssetPath = currentPath / "assets";
            EE_CORE_INFO("Asset folder found at: {}", s_AssetPath.string());
        }
        else
        {
            EE_CORE_WARN("Could not find asset folder within {} parent levels!", maxDepth);
        }



    }

    std::filesystem::path AssetManager::GetAssetPath(const std::string& subPath)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        std::filesystem::path path = s_AssetPath / subPath;
        return path.lexically_normal();  // Ensures a consistent format
    }

    std::filesystem::path AssetManager::GetScenePath(const std::string& sceneName)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);

        std::filesystem::path sceneDirectory = s_AssetPath / "scenes";

        for (const auto& entry : std::filesystem::recursive_directory_iterator(sceneDirectory))
        {
            if (entry.is_regular_file() && entry.path().stem() == sceneName) // Compare file name without extension
            {
                return entry.path().lexically_normal();
            }
        }

        return {};
    }


    std::filesystem::path AssetManager::GetAssetFolderPath()
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        return s_AssetPath;
    }

    std::filesystem::path AssetManager::GetCacheDirectory()
    {
        return GetAssetPath("cache/shader/opengl");
    }

    std::filesystem::path AssetManager::GetVulkanCacheDirectory()
    {
        return GetAssetPath("cache/shader/vulkan");
    }

    void AssetManager::CreateCacheDirectoryIfNeeded()
    {
        std::filesystem::path cacheDirectory = GetCacheDirectory();
        if (!std::filesystem::exists(cacheDirectory))
        {
            std::filesystem::create_directories(cacheDirectory);
            EE_CORE_INFO("Created cache directory: {}", cacheDirectory.string());
        }
    }


   
    std::vector<char> AssetManager::ReadFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();
        return buffer;
    }

    Ref<VulkanTexture> AssetManager::AddTexture(const std::string& name, const std::string& path, bool imGuiTexture, uint32_t textureID)
    {
		//std::lock_guard<std::mutex> lock(s_Mutex);
		if (s_textureCache.find(name) == s_textureCache.end())
		{
            s_textureCache[name] = std::make_shared<VulkanTexture>(path, name, imGuiTexture, textureID);
			EE_CORE_INFO("Texture added to cache: {}", name);
		}
		else
		{
			EE_CORE_WARN("Texture {} already exists in cache!", name);
		}
        return GetTexture(name);
    }


    
    Ref<VulkanTexture> AssetManager::AddTextureToCache(const std::string& name, Ref<VulkanTexture> texture)
    {

        if (!texture)
        {
            EE_CORE_WARN("Attempted to add null texture '{}' to cache", name);
            return nullptr;
        }

        auto it = s_textureCache.find(name);
        if (it == s_textureCache.end())
        {
            s_textureCache[name] = texture;
            EE_CORE_INFO("Texture '{}' added to cache", name);
        }
        else
        {
            EE_CORE_WARN("Texture '{}' already exists in cache, skipping", name);
        }

        return s_textureCache[name];
    }
    


    Ref<VulkanPixelTexture> AssetManager::AddPixelTexture(const std::string& name, const std::string& path)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        if (s_pixelTextureCache.find(name) == s_pixelTextureCache.end())
        {
            s_pixelTextureCache[name] = std::make_shared<VulkanPixelTexture>(path);
            EE_CORE_INFO("Texture added to cache: {}", name);
        }
        else
        {
            EE_CORE_WARN("Texture {} already exists in cache!", name);
        }
        return GetPixelTexture(name);
    }

    std::vector<Ref<VulkanTexture>> AssetManager::GetAllTextures()
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        std::vector<Ref<VulkanTexture>> textures;
        for (const auto& pair : s_textureCache)
        {
            textures.push_back(pair.second);
        }
        return textures;
    }


    Ref<VulkanTexture> AssetManager::GetTexture(const std::string& name)
    {
       // std::lock_guard<std::mutex> lock(s_Mutex);

        auto it = s_textureCache.find(name);
        if (it != s_textureCache.end())
        {
            return it->second; // Return the shared_ptr directly
        }
        else
        {
            EE_CORE_WARN("Texture '{}' not found in cache!", name);
            return nullptr;
        }
    }

    Ref<VulkanTexture> AssetManager::CloneTexture(const std::string& name)
    {
        // std::lock_guard<std::mutex> lock(s_Mutex);

        auto it = s_textureCache.find(name);
        if (it != s_textureCache.end())
        {
            return it->second->Clone();
        }
        else
        {
            EE_CORE_WARN("Texture '{}' not found in cache!", name);
            return nullptr;
        }
    }

    Ref<VulkanPixelTexture> AssetManager::GetPixelTexture(const std::string& name)
    {
        auto it = s_pixelTextureCache.find(name);
        if (it != s_pixelTextureCache.end())
        {
            return it->second; // Return the shared_ptr directly
        }
        else
        {
            EE_CORE_WARN("Pixel Texture {} not found in cache!", name);
            return nullptr;
        }
    }

    bool AssetManager::ExtractPixelsFromTilePallette(const glm::vec4& uv, std::vector<uint8_t>& outPixelData,
        int& outWidth, int& outHeight)
    {

		Ref<VulkanTexture> texture = GetTexture("tilePalette");

        const std::vector<uint8_t>& pixelData = texture->GetPixelData();
        if (pixelData.empty()) 
        {
            EE_CORE_ERROR("Texture  has no CPU-side pixel data!");
            return false;
        }

        uint32_t texWidth = texture->GetWidth();
        uint32_t texHeight = texture->GetHeight();
        constexpr uint32_t channels = 4; // Assuming RGBA8 format

        // Convert normalized UV to absolute pixel coordinates
        uint32_t x0 = static_cast<uint32_t>(uv.x * texWidth);
        uint32_t y0 = static_cast<uint32_t>(uv.y * texHeight);
        uint32_t x1 = static_cast<uint32_t>(uv.z * texWidth);
        uint32_t y1 = static_cast<uint32_t>(uv.w * texHeight);

        // Sanity check
        if (x1 <= x0 || y1 <= y0 || x1 > texWidth || y1 > texHeight)
        {
            EE_CORE_ERROR("Invalid UV bounds for extraction: ({}, {}, {}, {})", uv.x, uv.y, uv.z, uv.w);
            return false;
        }

        outWidth = x1 - x0;
        outHeight = y1 - y0;
        outPixelData.resize(outWidth * outHeight * channels);

		// flip the extracted region horizontally and vertically
        for (uint32_t y = 0; y < outHeight; ++y) {
            for (uint32_t x = 0; x < outWidth; ++x) {
                size_t srcX = x1 - 1 - x; // flip X
                size_t srcY = y1 - 1 - y; // flip Y
                size_t srcIndex = (srcY * texWidth + srcX) * channels;

                size_t dstIndex = (y * outWidth + x) * channels;
                memcpy(&outPixelData[dstIndex], &pixelData[srcIndex], channels);
            }
        }

        return true;
    }



    bool AssetManager::GetTexturePixelData(const std::string& textureName, std::vector<uint8_t>& outPixels, int& outWidth, int& outHeight)
    {

		if (textureName.empty())
		{
			EE_CORE_ERROR("Texture file does not exist: {}", textureName);
			return false;
		}
        
        std::string texturePath = ResolveTexturePath(textureName);

        int channels;
        //stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(texturePath.c_str(), &outWidth, &outHeight, &channels, STBI_rgb_alpha);
        if (!data)
        {
            EE_CORE_ERROR("Failed to load texture: {}", texturePath);
            return false;
        }

        size_t pixelCount = outWidth * outHeight;
        outPixels.resize(pixelCount * 4); // RGBA8 = 4 bytes per pixel
        std::memcpy(outPixels.data(), data, outPixels.size());

        stbi_image_free(data);
        return true;
    }

    std::string AssetManager::ResolveTexturePath(const std::string& textureName)
    {
        namespace fs = std::filesystem;
        fs::path base = GetAssetFolderPath() / "textures";

        // 1. Check base folder
        fs::path directPath = base / (textureName + ".png");
        if (fs::exists(directPath))
            return directPath.string();

        // 2. Check in 'tiles' subfolder
        fs::path tilePath = base / "tiles" / (textureName + ".png");
        if (fs::exists(tilePath))
            return tilePath.string();

        // 3. (Optional) Search recursively in all subfolders of textures/
        for (auto& p : fs::recursive_directory_iterator(base))
        {
            if (p.path().filename() == textureName + ".png")
                return p.path().string();
        }

        // 4. Not found
        EE_CORE_WARN("Texture not found: {}", textureName);
        return ""; // or return some fallback path
    }


}
