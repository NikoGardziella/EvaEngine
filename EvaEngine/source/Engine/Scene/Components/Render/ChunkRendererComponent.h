#pragma once

#include <glm/ext/vector_float2.hpp>
#include <Engine/Platform/Vulkan/VulkanTexture.h>
#include "Engine/Core/Core.h"

namespace Engine {

    struct ChunkRendererComponent
    {
        Ref<VulkanTexture> Texture;
        Ref<VulkanTexture> HealthTexture;
        glm::ivec2 ChunkCoords;
        float ChunkSize;
        bool IsLoaded;
	};
}

