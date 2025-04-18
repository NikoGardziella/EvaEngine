#include "pch.h"
#include "AssetManager.h"
#include <iostream>
#include <optional>
#include <mutex>

namespace Engine {

    std::filesystem::path AssetManager::s_AssetPath = "";

    // prevent multiple threads from accessing shared resources simultaneously
    std::mutex AssetManager::s_Mutex;
    std::unordered_map<std::string, std::shared_ptr<VulkanTexture>> AssetManager::s_textureCache;
    VkDeviceSize AssetManager::s_totalTextureMemory;



    void AssetManager::Initialize(int maxDepth)
    {
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

    Ref<VulkanTexture> AssetManager::AddTexture(const std::string& name, const std::string& path, bool imGuiTexture)
    {
		std::lock_guard<std::mutex> lock(s_Mutex);
		if (s_textureCache.find(name) == s_textureCache.end())
		{
            s_textureCache[name] = std::make_shared<VulkanTexture>(path, imGuiTexture);
			EE_CORE_INFO("Texture added to cache: {}", name);
		}
		else
		{
			EE_CORE_WARN("Texture {} already exists in cache!", name);
		}
        return GetTexture(name);
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
            EE_CORE_WARN("Texture {} not found in cache!", name);
            return nullptr;
        }
    }

}
