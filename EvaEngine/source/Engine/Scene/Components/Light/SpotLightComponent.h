#pragma once
namespace Engine {

    struct SpotLightComponent
    {
        glm::vec3 directionWS{ 0.0f, -1.0f, 0.0f }; // normalized
        glm::vec3 color{ 1.0f };

        float intensity = 1.0f;
        float range = 15.0f;

        float innerAngleRad = glm::radians(12.0f);
        float outerAngleRad = glm::radians(20.0f);
    };
}