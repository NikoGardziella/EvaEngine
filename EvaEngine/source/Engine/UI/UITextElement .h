#pragma once
#include "Font.h"
#include "UIElement.h"

namespace Engine {

    struct UITextElement : UIElement
    {
        Ref<Font> font;
        std::string text;
        glm::vec4 color = { 1,1,1,1 };
        float scale = 1.0f;

        void Draw() override
        {
            if (!tr.visible || !font || text.empty()) return;
            VulkanUIRenderer::DrawText(font, text, tr.posPx, color, scale);
        }
    };

}