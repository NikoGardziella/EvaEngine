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
        glm::vec2 WorldPos;
		glm::vec4 UV; // (u0,v0,u1,v1) for the tile in normalized UV space
        std::string TextureName;
    };
}

