#pragma once

#include <glm/ext/vector_float2.hpp>
#include <Engine/Platform/Vulkan/VulkanTexture.h>
#include "Engine/Core/Core.h"

namespace Engine {

    struct ChunkRendererComponent
    {
        Ref<VulkanTexture> Texture;
        Ref<VulkanTexture> HealthTexture;
        Ref<VulkanTexture> TerrainTexture;
        glm::ivec2 ChunkCoords;
        bool IsLoaded;
	};
}

