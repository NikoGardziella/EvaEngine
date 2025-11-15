#pragma once
#include "glm/glm.hpp"

namespace Engine {

    struct DirectionalLightComponent
    {
        glm::vec3 directionWS{ -0.3f, -1.0f, -0.2f };
        glm::vec3 color{ 1.0f };
        float intensity = 1.0f;
    };
}

