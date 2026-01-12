#pragma once
#include "UIElement.h"
#include <glm/fwd.hpp>

namespace Engine {

    struct UIContainer : UIElement
    {
        std::vector<UIElement*> children;
        glm::vec2 paddingPx{ 0,0 };
        glm::vec2 spacingPx{ 0,0 };

        enum class Layout { None, Vertical, Horizontal };
        Layout layout = Layout::None;

        void Draw() override {}
    };

}