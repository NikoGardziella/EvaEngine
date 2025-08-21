#include "pch.h"
#include "AssetManager.h"
#include <iostream>
#include <mutex>
#include <Engine/Platform/Vulkan/Pixel/VulkanPixelTexture.h>
#include "Engine/Platform/Vulkan/VulkanUtils.h"

#include <stb_image.h>



namespace Engine {

    std::filesystem::path AssetManager::s_AssetPath = "";

    // prevent multiple threads from accessing shared resources simultaneously
    std::mutex AssetManager::s_Mutex;
    std::unordered_map<std::string, std::shared_ptr<VulkanTexture>> AssetManager::s_textureCache;
    std::unordered_map<std::string, std::shared_ptr<VulkanPixelTexture>> AssetManager::s_pixelTextureCache;
    VkDeviceSize AssetManager::s_totalTextureMemory;

    std::unordered_map<eTileCategory, std::unordered_map<std::string, glm::vec4>> AssetManager::s_tileUVMapsByCategory;
    std::unordered_map<eTileCategory, std::vector<std::string>> AssetManager::s_tileNamesByCategory;
    std::unordered_map<eTileCategory, Ref<VulkanTexture>> AssetManager::s_tileAtlasesByCategory;
    std::unordered_map<std::string, glm::vec4> AssetManager::s_tileUVMap;

    std::unordered_map<eTileCategory, std::unordered_map<eTileMaterial, std::vector<std::string>>> AssetManager::s_tileNamesByCategoryAndMaterial;

    Ref<VulkanTexture> AssetManager::s_tileTextureIconAtlas;
    


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
            s_textureCache[name] = std::make_shared<VulkanTexture>(path, VK_FORMAT_R8G8B8A8_UNORM, name, imGuiTexture, textureID);
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

    bool AssetManager::ExtractPixelsFromTilePallette(const TileInfo& tile, std::vector<uint8_t>& outPixelData,
        int& outWidth, int& outHeight)
    {
        Ref<VulkanTexture> texture = GetTileTextureIconAtlas();
        bool flipVertical = true;
        bool flipHorizontal = false;
        const glm::vec4 uv = tile.UV;

        const std::vector<uint8_t>& pixelData = texture->GetPixelData();
        if (pixelData.empty())
        {
            EE_CORE_ERROR("Texture has no CPU-side pixel data!");
            return false;
        }

        uint32_t texWidth = texture->GetWidth();
        uint32_t texHeight = texture->GetHeight();
        constexpr uint32_t channels = 4; // Assuming RGBA8 format

        // Convert normalized UVs to absolute pixel coordinates
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

        for (uint32_t y = 0; y < outHeight; ++y) {
            for (uint32_t x = 0; x < outWidth; ++x) {
                uint32_t srcX = flipHorizontal ? (x1 - 1 - x) : (x0 + x);
                uint32_t srcY = flipVertical ? (y1 - 1 - y) : (y0 + y);

                size_t srcIndex = (srcY * texWidth + srcX) * channels;
                size_t dstIndex = (y * outWidth + x) * channels;

                memcpy(&outPixelData[dstIndex], &pixelData[srcIndex], channels);
            }
        }

        return true;
    }


    bool AssetManager::ExtractPixelsFromTilePallette(const TileInfo& tile, std::vector<uint8_t>& outPixelData,
        std::vector<uint8_t>& outHealthData, int& outWidth, int& outHeight)
    {
        Ref<VulkanTexture> texture = GetTileTextureIconAtlas();
        bool flipVertical = true;
        bool flipHorizontal = false;
        
        const glm::vec4& uv = tile.UV;
        const std::vector<uint8_t>& pixelData = texture->GetPixelData();
        if (pixelData.empty())
        {
            EE_CORE_ERROR("Texture has no CPU-side pixel data!");
            return false;
        }

        uint32_t texWidth = texture->GetWidth();
        uint32_t texHeight = texture->GetHeight();
        constexpr uint32_t channels = 4; // RGBA8 format

        // Convert normalized UVs to absolute pixel coordinates
        uint32_t x0 = static_cast<uint32_t>(uv.x * texWidth);
        uint32_t y0 = static_cast<uint32_t>(uv.y * texHeight);
        uint32_t x1 = static_cast<uint32_t>(uv.z * texWidth);
        uint32_t y1 = static_cast<uint32_t>(uv.w * texHeight);

        if (x1 <= x0 || y1 <= y0 || x1 > texWidth || y1 > texHeight)
        {
            EE_CORE_ERROR("Invalid UV bounds for extraction: ({}, {}, {}, {})", uv.x, uv.y, uv.z, uv.w);
            return false;
        }

        outWidth = x1 - x0;
        outHeight = y1 - y0;

        const size_t pixelCount = outWidth * outHeight;
        outPixelData.resize(pixelCount * channels);
        outHealthData.resize(pixelCount, 0);

        for (uint32_t y = 0; y < outHeight; ++y)
        {
            for (uint32_t x = 0; x < outWidth; ++x) 
            {
                uint32_t srcX = flipHorizontal ? (x1 - 1 - x) : (x0 + x);
                uint32_t srcY = flipVertical ? (y1 - 1 - y) : (y0 + y);

                size_t srcIndex = (srcY * texWidth + srcX) * channels;
                size_t dstIndex = (y * outWidth + x) * channels;

                // Copy RGBA
                memcpy(&outPixelData[dstIndex], &pixelData[srcIndex], channels);

                
                uint8_t alpha = pixelData[srcIndex + 3];
                outHealthData[y * outWidth + x] = (alpha > 0) ? tile.TileHealth : 0;
            }
        }

        return true;
    }


    bool AssetManager::GetTexturePixelData(
        const std::string& textureName,
        std::vector<uint8_t>& outPixels,
        std::vector<uint8_t>& outHealthData,
        int& outWidth,
        int& outHeight)
    {
        if (textureName.empty())
        {
            EE_CORE_ERROR("Texture file does not exist: {}", textureName);
            return false;
        }

        std::string texturePath = ResolveTexturePath(textureName);

        int channels;
        unsigned char* data = stbi_load(texturePath.c_str(), &outWidth, &outHeight, &channels, STBI_rgb_alpha);
        if (!data)
        {
            EE_CORE_ERROR("Failed to load texture: {}", texturePath);
            return false;
        }

        size_t pixelCount = outWidth * outHeight;

        // Copy RGBA pixels
        outPixels.resize(pixelCount * 4);
        std::memcpy(outPixels.data(), data, outPixels.size());

        // Fill health only for visible pixels
        outHealthData.resize(pixelCount);
        uint32_t health = 2;
        for (size_t i = 0; i < pixelCount; ++i)
        {
            uint8_t alpha = data[i * 4 + 3]; // Alpha channel
            outHealthData[i] = (alpha > 0) ? health : 0;
        }

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

    const std::vector<std::string>& AssetManager::GetTileNamesByCategory(eTileCategory category)
    {
        static const std::vector<std::string> empty;

        auto it = s_tileNamesByCategory.find(category);
        if (it != s_tileNamesByCategory.end())
            return it->second;

        EE_CORE_WARN("Requested tile names for unknown category.");
        return empty;
    }


    void AssetManager::CreateTileAtlas()
    {
        EE_PROFILE_FUNCTION();
        LoadTileProperties();
        namespace fs = std::filesystem;

        static const std::unordered_map<eTileCategory, std::string> CategoryNames = {
            { eTileCategory::Buildings, "buildings" },
            { eTileCategory::Terrain,   "terrain" },
            { eTileCategory::Roofs,     "roofs" },
            { eTileCategory::Vehicles,  "vehicles" }
        };

        const fs::path baseTilePath = AssetManager::GetAssetPath("textures/tiles");

        struct TileInfo {
            fs::path path;
            int width = 0, height = 0;
            stbi_uc* pixels = nullptr;
            eTileCategory category = eTileCategory::Terrain;
            eTileMaterial material = eTileMaterial::Default;
            std::string name;
        };

        std::vector<TileInfo> loadedTiles;

        // Load all tiles
        for (const auto& [category, folderName] : CategoryNames)
        {
            fs::path categoryPath = baseTilePath / folderName;
            if (!fs::exists(categoryPath) || !fs::is_directory(categoryPath))
            {
                EE_CORE_WARN("Tile category folder does not exist: {}", categoryPath.string());
                continue;
            }

            bool hasSubfolders = false;
            for (const auto& entry : fs::directory_iterator(categoryPath))
                if (entry.is_directory()) { hasSubfolders = true; break; }

            auto loadPng = [&](const fs::path& p, eTileCategory cat, eTileMaterial mat)
                {
                    if (p.extension() != ".png") return;
                    int w = 0, h = 0, channels = 0;
                    stbi_uc* pixels = stbi_load(p.string().c_str(), &w, &h, &channels, STBI_rgb_alpha);
                    if (!pixels)
                    {
                        EE_CORE_WARN("Failed to load tile '{}'", p.string());
                        return;
                    }
                    std::string name = p.stem().string();

                    // Optional: validate iso size
                    if (w != 128 || h != 256)
                        EE_CORE_WARN("Tile '{}' is {}x{}, expected 128x256 (iso). It will still be packed.", name, w, h);

                    loadedTiles.push_back({ p, w, h, pixels, cat, mat, name });
                };

            if (hasSubfolders)
            {
                for (const auto& subdir : fs::directory_iterator(categoryPath))
                {
                    if (!subdir.is_directory()) continue;
                    eTileMaterial mat = ParseMaterialFromPath(subdir.path());
                    for (const auto& file : fs::directory_iterator(subdir.path()))
                        loadPng(file.path(), category, mat);
                }
            }
            else
            {
                for (const auto& file : fs::directory_iterator(categoryPath))
                    loadPng(file.path(), category, eTileMaterial::Default);
            }
        }

        if (loadedTiles.empty())
        {
            EE_CORE_WARN("No tile images found in any category.");
            return;
        }

        // Stable atlas order
        std::sort(loadedTiles.begin(), loadedTiles.end(),
            [](const TileInfo& a, const TileInfo& b)
            {
                if (a.category != b.category) return a.category < b.category;
                if (a.material != b.material) return a.material < b.material;
                return a.name < b.name;
            });

        // Reset outputs (keep s_tileProperties from LoadTileProperties)
        s_tileUVMap.clear();
        s_tileUVMapsByCategory.clear();
        s_tileNamesByCategory.clear();
        s_tileNamesByCategoryAndMaterial.clear();

        // Packing params
        constexpr int ATLAS_W = 1024;   // 8 iso cells per row at 128px width
        constexpr int GUTTER = 2;      // px between tiles to prevent bleeding
        const int atlasWidth = ATLAS_W;

        // First pass: estimate atlas height with gutter
        int currentX = GUTTER;
        int currentY = GUTTER;
        int rowHeight = 0;
        for (const auto& t : loadedTiles)
        {
            if (currentX + t.width + GUTTER > atlasWidth)
            {
                currentY += rowHeight + GUTTER;
                currentX = GUTTER;
                rowHeight = 0;
            }
            rowHeight = std::max(rowHeight, t.height);
            currentX += t.width + GUTTER;
        }
        int atlasHeight = currentY + rowHeight + GUTTER;

        std::vector<uint8_t> atlasData(size_t(atlasWidth) * size_t(atlasHeight) * 4, 0);

        // Second pass: copy with gutter and compute UVs (inset by 0.5px)
        currentX = GUTTER;
        currentY = GUTTER;
        rowHeight = 0;

        for (const auto& t : loadedTiles)
        {
            if (currentX + t.width + GUTTER > atlasWidth)
            {
                currentY += rowHeight + GUTTER;
                currentX = GUTTER;
                rowHeight = 0;
            }

            // Copy pixels
            for (int y = 0; y < t.height; ++y)
            {
                const size_t dstY = size_t(currentY + y);
                const size_t srcY = size_t(y);
                uint8_t* dst = &atlasData[(dstY * size_t(atlasWidth) + size_t(currentX)) * 4];
                const uint8_t* src = &t.pixels[(srcY * size_t(t.width)) * 4];
                memcpy(dst, src, size_t(t.width) * 4);
            }

            // Compute UVs with a tiny 0.5px inset (stays inside the copied rect)
            const float invW = 1.0f / float(atlasWidth);
            const float invH = 1.0f / float(atlasHeight);
            const float inset = 0.5f;

            float u0 = (float(currentX) + inset) * invW;
            float v0 = (float(currentY) + inset) * invH;
            float u1 = (float(currentX + t.width) - inset) * invW;
            float v1 = (float(currentY + t.height) - inset) * invH;

            glm::vec4 uv = glm::vec4(u0, v0, u1, v1);

            const std::string name = t.name;

            // Store UVs
            s_tileUVMap[name] = uv;
            s_tileUVMapsByCategory[t.category][name] = uv;

            // Merge/override tile properties
            TileProperties props{};
            if (auto it = s_tileProperties.find(name); it != s_tileProperties.end())
                props = it->second;
            props.name = name;
            props.uv = uv;
            // If no saved material, use folder material
            if (props.material == eTileMaterial::None)
                props.material = t.material;

            s_tileProperties[name] = props;

            // Names per category
            s_tileNamesByCategory[t.category].push_back(name);

            // Category+material list with fallback
            const eTileMaterial matToUse =
                (props.material != eTileMaterial::None) ? props.material : t.material;
            s_tileNamesByCategoryAndMaterial[t.category][matToUse].push_back(name);

            currentX += t.width + GUTTER;
            rowHeight = std::max(rowHeight, t.height);

            stbi_image_free(t.pixels);
        }

        // Create Vulkan texture atlas
        s_tileTextureIconAtlas = std::make_shared<VulkanTexture>(
            atlasWidth, atlasHeight, VK_FORMAT_R8G8B8A8_UNORM, "combined_tileAtlas", true);

        // IMPORTANT: layout transitions in correct order
        // If SetData handles transitions internally, just call SetData and skip manual transitions.
        // Otherwise do UNDEFINED -> TRANSFER_DST -> SHADER_READ_ONLY.
        // Example (uncomment if SetData does NOT transition):
        s_tileTextureIconAtlas = std::make_shared<VulkanTexture>(atlasWidth, atlasHeight, VK_FORMAT_R8G8B8A8_UNORM,
            "combined_tileAtlas", true);
        VulkanUtils::TransitionImageLayout(s_tileTextureIconAtlas->GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        s_tileTextureIconAtlas->SetData(atlasData.data(), static_cast<uint32_t>(atlasData.size()));
        s_tileTextureIconAtlas->SetPixelData(atlasData);
        EE_CORE_INFO("Created combined tile atlas with dimensions {}x{}", atlasWidth, atlasHeight);
    }





    eTileMaterial AssetManager::ParseMaterialFromPath(const std::filesystem::path& path)
    {
        std::string materialName = path.filename().string(); // e.g., "wood", "concrete"

        if (materialName == "default" || materialName == "Default") return eTileMaterial::Default;
        if (materialName == "wood" || materialName == "Wood") return eTileMaterial::Wood;
        if (materialName == "concrete" || materialName == "Concrete") return eTileMaterial::Concrete;
        // Add more as needed

        return eTileMaterial::Undefined; // or Default
    }

    const std::vector<std::string>& AssetManager::GetTileNamesByCategoryAndMaterial(eTileCategory category, eTileMaterial material)
    {
        static const std::vector<std::string> empty;
        auto catIt = s_tileNamesByCategoryAndMaterial.find(category);
        if (catIt == s_tileNamesByCategoryAndMaterial.end())
        {
            return empty;
        }

        auto matIt = catIt->second.find(material);
        if (matIt == catIt->second.end())
        {
            return empty;
        }
        return matIt->second;
    }


    int AssetManager::GetTileCountForCategory(eTileCategory category)
    {
        auto it = s_tileNamesByCategory.find(category);
        if (it != s_tileNamesByCategory.end())
            return static_cast<int>(it->second.size());

        return 0;
    }

    const TileProperties& AssetManager::GetTileProperties(const std::string& tileName)
    {
       
        auto it = s_tileProperties.find(tileName);
        if (it != s_tileProperties.end())
            return it->second;

        static TileProperties s_default;
        return s_default;
    }

    void AssetManager::LoadTileProperties()
    {
        std::unordered_map<std::string, TileProperties> loadedTiles;
        TileSerializer::Load(loadedTiles);  

        if (loadedTiles.empty())
        {
            EE_CORE_WARN("Tile properties file is empty or failed to load.");
        }
        else
        {
            s_tileProperties = std::move(loadedTiles);
        }
    }


}
 