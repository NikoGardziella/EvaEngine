#pragma once
#include <vector>

#include "UIElement.h"

namespace Engine {

    struct UIContext {

        std::vector<Ref<UIElement>> elements;

        template<typename T, typename... Args>
        T& Add(Args&&... args)
        {
            auto e = std::make_unique<T>(std::forward<Args>(args)...);
            T& ref = *e;
            elements.push_back(std::move(e));
            return ref;
        }

        void Clear()
        {
            elements.clear();
        }
    };

}
