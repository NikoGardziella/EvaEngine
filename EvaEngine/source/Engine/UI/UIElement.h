#pragma once
#include "UITransform2D.h"
#include "Engine/Core/Core.h"
#include <Engine/Platform/Vulkan/VulkanTexture.h>
#include <Engine/Renderer/UI/VulkanUIRenderer.h>

namespace Engine {

    struct UIElement
    {
        UITransform2D tr;
        virtual ~UIElement() = default;
        virtual void Draw() = 0;
    };

    struct UIImageElement : UIElement
    {
        Ref<VulkanTexture> texture;
        glm::vec4 tint = { 1,1,1,1 };


       

        void Draw() override
        {
            if (!tr.visible || !texture) return;

            VulkanUIRenderer::DrawUIIcon(texture, tr,tint);
        }
    };

}
