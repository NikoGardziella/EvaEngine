#pragma once
#include "glm/glm.hpp"

namespace Engine
{
    struct PointLightComponent
    {
        glm::vec3 positionWS{ 0 };
        glm::vec3 color{ 1.0f };
        float intensity = 1.0f;
        float radius = 10.0f;
    };

}