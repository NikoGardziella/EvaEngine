#include "pch.h"
#include "AssetManager.h"
#include <iostream>
#include <mutex>
#include <Engine/Platform/Vulkan/Pixel/VulkanPixelTexture.h>
#include "Engine/Platform/Vulkan/VulkanUtils.h"
#include "Engine/AssetManager/Utils/AssetManagerUtils.h"
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
    // Extracts a tile from the atlas and produces:
//  - outPixelData : RGBA8 color (as before)
//  - outHealthData: RGBA8 UINT where R=health, B=collision mask, G/A unused (0)
//
// Conventions:
//  - Row 0 is the bottom (kFlipVertical = true)
//  - Foot band is BELOW the pivot row (typical “feet”). Flip if your assets expect above-pivot.
   
    bool AssetManager::ExtractPixelsFromTilePallette(
        const TileInfo& tile,
        std::vector<uint8_t>& outPixelData,   // RGBA8, row 0 = bottom
        std::vector<uint8_t>& outHealthData,  // RGBA8: R=health, B=mask, G/A=0
        int& outWidth, int& outHeight)
    {
        Ref<VulkanTexture> atlas = GetTileTextureIconAtlas();
        const auto& pixels = atlas->GetPixelData();
        if (pixels.empty()) {
            EE_CORE_ERROR("Atlas has no CPU pixels");
            return false;
        }

        const TileProperties props = AssetManager::GetTileProperties(tile.name);
        const auto& pr = props.pixelRect; // x,y,w,h in atlas space

        const int x0 = pr.x;
        const int y0 = pr.y;
        const int w = pr.w;
        const int h = pr.h;

        if (w <= 0 || h <= 0 ||
            x0 < 0 || y0 < 0 ||
            x0 + w > int(atlas->GetWidth()) ||
            y0 + h > int(atlas->GetHeight()))
        {
            EE_CORE_ERROR("Bad pixelRect for '{}'", tile.name.c_str());
            return false;
        }

        outWidth = w;
        outHeight = h;

        outPixelData.assign(size_t(w) * size_t(h) * 4, 0);
        outHealthData.assign(size_t(w) * size_t(h) * 4, 0);

        constexpr int kHealthChan = 0; // R
        constexpr int kMaskChan = 2; // B

        // Output is bottom-origin so it matches world grid used elsewhere
        constexpr bool kFlipVertical = true;
        constexpr bool kFlipHorizontal = false;

        const uint32_t A_W = atlas->GetWidth();
        const uint8_t* src = pixels.data();

        // Pivot row (from bottom). Clamp so we never go OOB.
        const int pivotY = std::clamp(int(props.pivotYOffsetPx), 0, h - 1);

        // Select the **foot band below the pivot** by default.
        const int footRowsBelowPivot = std::clamp(int(props.collisionFootRowsPx), 0, pivotY);
        const int bandStartY = std::max(0, pivotY - footRowsBelowPivot);
        const int bandEndY = pivotY; // [start, end)

        for (int y = 0; y < h; ++y)
        {
            const int sy = kFlipVertical ? (y0 + (h - 1 - y)) : (y0 + y);
            const size_t srcRow = size_t(sy) * size_t(A_W) * 4;
            const size_t dstRow = size_t(y) * size_t(w) * 4;
            const bool inFootBandY = (y >= bandStartY && y < bandEndY);

            for (int x = 0; x < w; ++x)
            {
                const int sx = kFlipHorizontal ? (x0 + (w - 1 - x)) : (x0 + x);
                const size_t si = srcRow + size_t(sx) * 4;
                const size_t di = dstRow + size_t(x) * 4;

                // Copy color straight
                outPixelData[di + 0] = src[si + 0];
                outPixelData[di + 1] = src[si + 1];
                outPixelData[di + 2] = src[si + 2];
                outPixelData[di + 3] = src[si + 3];

                // Health R: by default use tile health wherever alpha > 0
                const uint8_t a = src[si + 3];
                uint8_t healthR = (a > 0) ? uint8_t(tile.TileHealth) : uint8_t(0);

                // Collision mask B: independent of alpha, based purely on the foot band
                uint8_t maskB = 0;
                if (inFootBandY) {
                    maskB = 255u;

                    // Ensure collidable even if alpha==0 in this band
                    if (healthR == 0) healthR = uint8_t(tile.TileHealth);
                }

                outHealthData[di + kHealthChan] = healthR; // R
                outHealthData[di + 1] = 0;       // G
                outHealthData[di + kMaskChan] = maskB;   // B
                outHealthData[di + 3] = 0;       // A
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

        for (auto& p : fs::recursive_directory_iterator(base))
        {
            if (p.path().filename() == textureName + ".png")
                return p.path().string();
        }

        // 4. Not found
        EE_CORE_WARN("Texture not found: {}", textureName);
        return ""; 
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
        constexpr int GUTTER = 1;      // px between tiles to prevent bleeding
        const int atlasWidth = ATLAS_W;

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

            
                for (int y = 0; y < t.height; ++y) {
                    const size_t dstY = size_t(currentY + y);
                    const uint8_t* src = &t.pixels[(size_t(y) * t.width) * 4];
                    uint8_t* dst = &atlasData[(dstY * size_t(atlasWidth) + size_t(currentX)) * 4];
                    memcpy(dst, src, size_t(t.width) * 4);
                }

                // UV with inset (for UI rendering)
                const float invW = 1.0f / float(atlasWidth);
                const float invH = 1.0f / float(atlasHeight);
                const float inset = 0.5f;

                glm::vec4 uv{
                    (float(currentX) + inset) * invW,
                    (float(currentY) + inset) * invH,
                    (float(currentX + t.width) - inset) * invW,
                    (float(currentY + t.height) - inset) * invH
                };

                const std::string name = t.name;
                int alphaThresh = 8;
                TileProperties props{};
                if (auto it = s_tileProperties.find(name); it != s_tileProperties.end())
                    props = it->second;

                if (props.pivotAuto)
                {
                    int autoPivotY = 0, autoPivotX = 0;
                    AssetManagerUtils::ComputePivotFromAlpha(t.pixels, t.width, t.height, alphaThresh, autoPivotY, autoPivotX);

                    props.pivotYOffsetPx = autoPivotY;
                    props.pivotXCenterOffsetPx = autoPivotX;
                }
                

                props.name = name;
                props.health = (props.health == 0 ? 1u : props.health);
                if (props.material == eTileMaterial::None) props.material = t.material;
                props.uv = uv;

                props.pixelRect = { currentX, currentY, t.width, t.height };


                if (props.collisionFootRowsPx == 0)
                {
                    switch (t.category)
                    {
                        case eTileCategory::Buildings: props.collisionFootRowsPx = 32; break;
                        case eTileCategory::Vehicles:  props.collisionFootRowsPx = 20; break;
                        case eTileCategory::Terrain:   props.collisionFootRowsPx = 0;  break;
                        case eTileCategory::Roofs:     props.collisionFootRowsPx = 0;  break;
                        default:                       props.collisionFootRowsPx = 0;  break;
                    }
                }


                s_tileProperties[name] = props;

                s_tileUVMap[name] = uv;
                s_tileUVMapsByCategory[t.category][name] = uv;
                s_tileNamesByCategory[t.category].push_back(name);
                s_tileNamesByCategoryAndMaterial[t.category][props.material].push_back(name);

                currentX += t.width + GUTTER;
                rowHeight = std::max(rowHeight, t.height);
                stbi_image_free(t.pixels);
            }

            s_tileTextureIconAtlas = std::make_shared<VulkanTexture>(
                atlasWidth, atlasHeight, VK_FORMAT_R8G8B8A8_UNORM, "combined_tileAtlas", /*generateMips*/ false
            );
            VulkanUtils::TransitionImageLayout(s_tileTextureIconAtlas->GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            s_tileTextureIconAtlas->SetData(atlasData.data(), static_cast<uint32_t>(atlasData.size()));

            // Keep CPU copy for extraction:
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
 