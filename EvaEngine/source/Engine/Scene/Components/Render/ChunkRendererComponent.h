#pragma once
#include <Engine/Platform/Vulkan/VulkanTexture.h>

#include "glm/glm.hpp"

namespace Engine {

    struct ChunkRendererComponent
    {
        Ref<VulkanTexture> Texture;
        Ref<VulkanTexture> PropertiesTexture;
        std::vector<uint8_t> HealthData;
        Ref<VulkanTexture> TerrainTexture;
        Ref<VulkanTexture> VisualEffectTexture;
        glm::ivec2 ChunkCoords;
        bool IsLoaded;
	};
}

