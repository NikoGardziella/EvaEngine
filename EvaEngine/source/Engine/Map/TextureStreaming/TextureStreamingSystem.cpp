#include "pch.h"
#include "TextureStreamingSystem.h"
#include <glm/geometric.hpp>
#include "Engine/AssetManager/AssetManager.h"
#include <Engine/Scene/Component.h>

namespace Engine {

    const int mapWidth = 4096; // Example width in pixels
    const int mapHeight = 4096; // Example height in pixels
    const int chunkSize = 256; // Size of each chunk in pixels
	TextureStreamingSystem::TextureStreamingSystem()
    {
      
    }

    TextureStreamingSystem::~TextureStreamingSystem()
    {

    }

    void TextureStreamingSystem::Update(const glm::vec2& playerPos, entt::registry& gameRegistry)
    {
        EE_PROFILE_FUNCTION();

        glm::ivec2 playerChunk = glm::floor(playerPos / float(CHUNK_SIZE));

        if (m_chunkMap.empty())
        {
            return;
        }

        // --- Load nearby chunks ---
        for (auto& [id, chunk] : m_chunkMap)
        {
            if (chunk.IsLoaded)
                continue;

            glm::ivec2 chunkCoords = chunk.ChunkCoords;
            float dist = glm::length(glm::vec2(chunkCoords - playerChunk));
            if (dist <= LOAD_RADIUS)
            {
                LoadChunkToGPU(chunk, gameRegistry);
            }
        }

        // --- Unload far-away chunks ---
        for (auto& [id, chunk] : m_chunkMap)
        {
            if (!chunk.IsLoaded)
                continue;

            glm::ivec2 chunkCoords = chunk.ChunkCoords;
            float dist = glm::length(glm::vec2(chunkCoords - playerChunk));

            if (dist > UNLOAD_RADIUS)
            {
                UnloadChunkFromGPU(chunk, gameRegistry);
            }
        }
    }


    void TextureStreamingSystem::UploadToChunkFromTexture(const glm::vec3& worldPosition, UUID ID, std::string name,
        const std::vector<uint8_t>& textureData, uint32_t textureWidth, uint32_t textureHeight)
    {

        EE_PROFILE_FUNCTION();

        //glm::ivec2 chunkCoords = glm::floor(glm::vec2(worldPosition.x, worldPosition.y) / glm::vec2(textureWidth, textureHeight));
        glm::ivec2 chunkCoords = glm::floor(glm::vec2(worldPosition.x, worldPosition.y) / float(CHUNK_SIZE));
        EE_CORE_INFO("Uploading chunk for ID at coords: {}, {}", chunkCoords.x, chunkCoords.y);

        // Always create or overwrite chunk by ID
        TextureChunk& chunk = m_chunkMap[ID];

        chunk.ID = ID;
        chunk.Name = name;
        chunk.IsLoaded = false;
        chunk.WorldPosition = glm::ivec2(worldPosition.x, worldPosition.y);
        chunk.ChunkCoords = chunkCoords;
        chunk.Width = textureWidth;
        chunk.Height = textureHeight;

        chunk.PixelData.resize(textureWidth * textureHeight * 4);

        // Copy pixel data directly
        std::memcpy(chunk.PixelData.data(), textureData.data(), textureData.size());
    }
    



    void TextureStreamingSystem::LoadChunkToGPU(TextureChunk& chunk, entt::registry& gameRegistry)
    {
        EE_PROFILE_FUNCTION();


        EE_CORE_INFO("Loading chunk at coords: {}, {}", chunk.ChunkCoords.x, chunk.ChunkCoords.y);

        if (chunk.PixelData.empty())
        {
			EE_CORE_ERROR("Chunk pixel data is empty for: {}", chunk.Name);
            return;
        }
        constexpr int CHUNK_RES = CHUNK_SIZE; // Assuming square chunks

        chunk.GPUTexture = AssetManager::CloneTexture(chunk.Name);
        EE_CORE_INFO("Texture size: {}x{}", chunk.GPUTexture->GetWidth(), chunk.GPUTexture->GetHeight());
		chunk.GPUTexture->SetData(chunk.PixelData.data(), chunk.GPUTexture->GetHeight() * chunk.GPUTexture->GetWidth() * 4);


        auto entityView = gameRegistry.view<IDComponent, SpriteRendererComponent>();

        for (auto entity : entityView)
        {

            auto [IDComp, spriteRendComp] = entityView.get<IDComponent, SpriteRendererComponent>(entity);
			if (IDComp.ID == chunk.ID)
			{
				spriteRendComp.Texture = chunk.GPUTexture;
				break;
			}

        }

        chunk.IsLoaded = true;
    }



    void TextureStreamingSystem::UnloadChunkFromGPU(TextureChunk& chunk, entt::registry& gameRegistry)
    {
        EE_CORE_INFO("Unloading chunk at coords: {}, {}", chunk.ChunkCoords.x, chunk.ChunkCoords.y);

        // Reset the owning smart pointer — its destructor will free Vulkan resources
        auto entityView = gameRegistry.view<IDComponent, SpriteRendererComponent>();

        for (auto entity : entityView)
        {

            auto [IDComp, spriteRendComp] = entityView.get<IDComponent, SpriteRendererComponent>(entity);
            if (IDComp.ID == chunk.ID)
            {
                spriteRendComp.Texture = nullptr;
                break;
            }

        }
        chunk.GPUTexture = nullptr;
        chunk.IsLoaded = false;
    }

    void TextureStreamingSystem::ResetAllChunks(entt::registry& gameRegistry)
    {
        EE_CORE_INFO("Resetting all chunks (scheduled unload)...");

        // Phase 1: gather IDs of all loaded chunks
        std::vector<UUID> toUnload;
		uint32_t chunkCount = static_cast<uint32_t>(m_chunkMap.size());
        if (m_chunkMap.empty() || chunkCount <= 0)
        {
			EE_CORE_WARN("No chunks to unload.");
			return;
        }

        toUnload.reserve(m_chunkMap.size());
        for (auto& [id, chunk] : m_chunkMap)
        {
            if (chunk.IsLoaded)
                toUnload.push_back(id);
        }

        // Phase 2: safely unload each, one by one
        for (auto const& id : toUnload)
        {
            auto it = m_chunkMap.find(id);
            if (it == m_chunkMap.end())
                continue;

            // Now it->second is guaranteed to exist, and no one else is iterating the map
            UnloadChunkFromGPU(it->second, gameRegistry);
        }
    }

}
