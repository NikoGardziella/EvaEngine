#pragma once
#include <glm/ext/vector_float2.hpp>
#include <Engine/Platform/Vulkan/VulkanTexture.h>
#include "Engine/Core/Core.h"

namespace Engine {

    struct TileComponent
    {
        uint32_t TileID;
        glm::vec2 GridPos;
        Ref<VulkanTexture> Texture;
        bool IsDestructible;
    };
}

