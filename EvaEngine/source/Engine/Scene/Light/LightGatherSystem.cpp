#include "pch.h"
#include "LightGatherSystem.h"

#include "Engine/Renderer/Lights/VulkanLighting.h"
#include <Engine/Scene/Components/Light/DirectionalLightComponent.h>
#include <Engine/Scene/Components/Light/PointLightComponent.h>
#include <Engine/Scene/Components/Light/SpotLightComponent.h>
#include <Engine/Scene/Entity.h>

namespace Engine {

    void LightGatherSystem::Update(Engine::Scene* scene, uint32_t frameIndex)
    {

        scene->ForEach<DirectionalLightComponent>([&](Entity e, DirectionalLightComponent& dl)
            {
                VulkanLighting::SubmitDirectional(dl.directionWS, dl.color, dl.intensity);
            });

        scene->ForEach<PointLightComponent, TransformComponent>([&](Entity e, PointLightComponent& pl, TransformComponent& transformComp)
            {

                VulkanLighting::SubmitPoint(transformComp.Translation, pl.color, pl.intensity, pl.radius);
            });

        scene->ForEach<SpotLightComponent, TransformComponent>([&](Entity e, SpotLightComponent& sl, TransformComponent& transformComp)
            {

                VulkanLighting::SubmitSpot(transformComp.Translation, sl.directionWS, sl.color,
                    sl.intensity, sl.range, sl.innerAngleRad, sl.outerAngleRad);
            });
    }

}