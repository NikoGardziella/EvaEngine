#pragma once
#include "Font.h"
#include "UIElement.h"
#include <cstdint>
#include <Engine/Core/Core.h>
#include <string>

namespace Engine {

    struct UITextElement : UIElement
    {
        Ref<Font>   font;
        std::string text;
        glm::vec4   color = { 1,1,1,1 };
        float       scale = 1.0f;
        uint32_t    layer;
        bool        visible;

        void Draw() override
        {
            if (!tr.visible || !font || text.empty()) return;
            VulkanUIRenderer::DrawUIText(font, text, tr, color, scale);
        }
    };

}