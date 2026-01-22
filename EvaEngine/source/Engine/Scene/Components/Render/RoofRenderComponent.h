#pragma once
#include <Engine/Platform/Vulkan/VulkanTexture.h>


namespace Engine {

    struct RoofRenderComponent
    {
        Ref<VulkanTexture> Texture;
        bool IsLoaded;
        glm::vec3 LocalOffset = glm::vec3(0.0f);
    };
}

