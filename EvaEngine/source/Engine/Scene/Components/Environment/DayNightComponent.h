#pragma once
#include <glm/glm.hpp>
#include <array>

namespace Engine {

    struct SunKeyframe
    {
        float time = 0.0f;
        glm::vec3 sunColor = glm::vec3(1.0f);
        float sunIntensity = 1.0f;
        glm::vec3 ambientColor = glm::vec3(0.2f);
        float ambientIntensity = 0.2f;
    };

    static constexpr int MAX_SUN_KEYFRAMES = 8;

    struct DayNightComponent
    {
        // Config
        float dayLengthSeconds = 600.0f;
        float timeScale = 1.0f;
        bool  paused = false;

        // Orbit
        float orbitTiltDeg = 30.0f;
        float sunMinAngleDeg = -10.0f;
        float sunMaxAngleDeg = 70.0f;

        // State
        float timeNormalized = 0.5f;

        // Keyframes (editable)
        int keyframeCount = 6;
        std::array<SunKeyframe, MAX_SUN_KEYFRAMES> keyframes = { {
            { 0.00f, { 0.05f, 0.05f, 0.10f }, 0.00f, { 0.02f, 0.02f, 0.05f }, 0.05f },
            { 0.20f, { 0.10f, 0.08f, 0.15f }, 0.00f, { 0.05f, 0.04f, 0.08f }, 0.08f },
            { 0.28f, { 0.90f, 0.50f, 0.20f }, 0.50f, { 0.30f, 0.20f, 0.15f }, 0.15f },
            { 0.50f, { 1.00f, 0.95f, 0.85f }, 1.00f, { 0.40f, 0.45f, 0.60f }, 0.20f },
            { 0.72f, { 0.95f, 0.40f, 0.10f }, 0.50f, { 0.30f, 0.15f, 0.10f }, 0.15f },
            { 0.80f, { 0.10f, 0.08f, 0.15f }, 0.00f, { 0.05f, 0.04f, 0.08f }, 0.08f },
        } };

        // Computed
        glm::vec3 sunDirection = glm::vec3(0.0f, -0.5f, -1.0f);
        glm::vec3 sunColor = glm::vec3(1.0f, 0.95f, 0.85f);
        float     sunIntensity = 1.0f;
        glm::vec3 ambientColor = glm::vec3(0.4f, 0.45f, 0.6f);
        float     ambientIntensity = 0.2f;
    };

} // namespace Engine