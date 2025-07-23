
#pragma once

#include <glm/glm.hpp>
#include <functional>

namespace Engine {

    struct IVec2Hasher
    {
        std::size_t operator()(const glm::ivec2& v) const noexcept
        {
            std::size_t h1 = std::hash<int>{}(v.x);
            std::size_t h2 = std::hash<int>{}(v.y);
            return h1 ^ (h2 << 1);
        }
    };

}