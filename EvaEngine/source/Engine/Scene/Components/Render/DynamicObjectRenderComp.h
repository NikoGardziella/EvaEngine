#pragma once
#include <Engine/Platform/Vulkan/VulkanTexture.h>


#include "glm/glm.hpp"

namespace Engine {

    struct DynamicObjectRenderComp
    {
        Ref<VulkanTexture> Texture;
        Ref<VulkanTexture> PropertiesTexture;
        bool IsLoaded;

        glm::vec2 OriginBLWorld = glm::vec2(0.0f);
        glm::vec2 WorldSize = glm::vec2(0.0f);
	};
}

