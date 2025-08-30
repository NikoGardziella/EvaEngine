#pragma once

#include <glm/ext/vector_float2.hpp>
#include <Engine/Platform/Vulkan/VulkanTexture.h>
#include "Engine/Core/Core.h"

namespace Engine {

    struct ChunkRendererComponent
    {
        Ref<VulkanTexture> Texture;
        Ref<VulkanTexture> PropertiesTexture;
        std::vector<uint8_t> HealthData;
        Ref<VulkanTexture> TerrainTexture;
        glm::ivec2 ChunkCoords;
        bool IsLoaded;
	};
}

