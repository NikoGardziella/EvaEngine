#pragma once
#include "glm/glm.hpp"

namespace Engine {

    struct RenderBoundsComponent
    {
        // local-space AABB of the combined submeshes
        glm::vec3 minL{ -0.5f }, maxL{ +0.5f };
    };


}