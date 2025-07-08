#pragma once
#include <glm/ext/vector_float2.hpp>
#include <Engine/Platform/Vulkan/VulkanTexture.h>
#include "Engine/Core/Core.h"

namespace Engine {

    struct TileInfo {
        glm::vec2 position; // local position within group
        glm::vec4 UV;
        std::string name;
        bool IsDestructible;
        bool IsRoof;
		TileInfo(const glm::vec2& pos = glm::vec2(0.0f), const glm::vec4& uvCoords = glm::vec4(0.0f), const std::string& tileName = "", bool destructible = false, bool roof = false)
			: position(pos), UV(uvCoords), name(tileName), IsDestructible(destructible), IsRoof(roof) {
		}
    };

    struct TileComponent
    {
        uint32_t TileID;
        Ref<VulkanTexture> Texture;
        glm::vec2 WorldPos;
        std::vector<TileInfo> tiles;
    };
}

