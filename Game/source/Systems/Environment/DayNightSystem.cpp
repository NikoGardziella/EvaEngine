#include "pch.h"
#include "DayNightSystem.h"

#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Entity.h>

#include <glm/gtc/constants.hpp>
#include <cmath>

#include "Engine/Scene/Components/Environment/DayNightComponent.h"
#include <Engine/Scene/Components/Light/DirectionalLightComponent.h>




void DayNightSystem::UpdateDayNightSystem(float deltaTime, Engine::Scene* scene)
{
    scene->ForEach<Engine::DayNightComponent>([&](Engine::Entity e, Engine::DayNightComponent& dayNightComp)
        {
            if (dayNightComp.paused || dayNightComp.dayLengthSeconds <= 0.0f)
                return;

            // Advance time
            float step = (deltaTime * dayNightComp.timeScale) / dayNightComp.dayLengthSeconds;
            dayNightComp.timeNormalized += step;

            if (dayNightComp.timeNormalized >= 1.0f)
            {
                dayNightComp.timeNormalized -= 1.0f;

            }
            if (dayNightComp.timeNormalized < 0.0f)
            {

                dayNightComp.timeNormalized += 1.0f;
            }

            ComputeSunDirection(dayNightComp);
            ComputeLightColors(dayNightComp);
            ApplyToDirectionalLight(scene, dayNightComp);
        });
}

void DayNightSystem::ComputeSunDirection(Engine::DayNightComponent& dn)
{
    float dayAngle = dn.timeNormalized * glm::two_pi<float>();

    // Elevation: 0 at midnight, 1 at noon
    float elevT = (std::sin(dayAngle - glm::half_pi<float>()) + 1.0f) * 0.5f;
    float elevDeg = glm::mix(dn.sunMinAngleDeg, dn.sunMaxAngleDeg, elevT);
    float elevRad = glm::radians(elevDeg);

    // rotates east-> west over the day
    float tiltRad = glm::radians(dn.orbitTiltDeg);

    float cosAz = std::cos(dayAngle);
    float sinAz = std::sin(dayAngle);

    // Sun direction points FROM sun TO scene
    glm::vec3 dir;
    dir.x = std::cos(elevRad) * sinAz;                          // east-west sway
    dir.y = std::cos(elevRad) * cosAz * std::sin(tiltRad);      // iso depth
    dir.z = -std::sin(elevRad);                                  // DOWN = strongest at noon

    if (glm::length(dir) > 0.001f)
    {
        dn.sunDirection = glm::normalize(dir);

    }
    else
    {
        dn.sunDirection = glm::vec3(0.0f, 0.0f, -1.0f);
    }

}
void DayNightSystem::ComputeLightColors(Engine::DayNightComponent& dn)
{
    float t = dn.timeNormalized;
    int count = dn.keyframeCount;

    int idxA = count - 1;
    int idxB = 0;

    for (int i = 0; i < count; ++i)
    {
        if (dn.keyframes[i].time > t)
        {
            idxB = i;
            idxA = (i == 0) ? count - 1 : i - 1;
            break;
        }
        if (i == count - 1)
        {
            idxA = i;
            idxB = 0;
        }
    }

    const Engine::SunKeyframe& a = dn.keyframes[idxA];
    const Engine::SunKeyframe& b = dn.keyframes[idxB];

    float range = b.time - a.time;
    float pos = t - a.time;
    if (range < 0.0f) range += 1.0f;
    if (pos < 0.0f) pos += 1.0f;

    float f = (range > 0.001f) ? (pos / range) : 0.0f;
    f = glm::clamp(f, 0.0f, 1.0f);
    f = f * f * (3.0f - 2.0f * f);

    dn.sunColor = glm::mix(a.sunColor, b.sunColor, f);
    dn.sunIntensity = glm::mix(a.sunIntensity, b.sunIntensity, f);
    dn.ambientColor = glm::mix(a.ambientColor, b.ambientColor, f);
    dn.ambientIntensity = glm::mix(a.ambientIntensity, b.ambientIntensity, f);
}
    
void DayNightSystem::ApplyToDirectionalLight(Engine::Scene* scene, const Engine::DayNightComponent& dn)
{
    scene->ForEach<Engine::DirectionalLightComponent>(
        [&](Engine::Entity e, Engine::DirectionalLightComponent& light)
        {
            light.directionWS = dn.sunDirection;
            light.color     = dn.sunColor;
            light.intensity = dn.sunIntensity;
        });
}


