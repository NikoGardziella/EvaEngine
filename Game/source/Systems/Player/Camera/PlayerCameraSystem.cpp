#include "PlayerCameraSystem.h"
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>
#include <Engine/Debug/Instrumentor.h>



void PlayerCameraSystem::UpdatePlayerCameraSystem(float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    // Primary camera
    Engine::Entity cameraEntity = scene->GetPrimaryCameraEntity();
    if (!cameraEntity)
    {
        return;
    }
    Engine::CameraComponent& cameraComp = cameraEntity.GetComponent<Engine::CameraComponent>();
    if (cameraComp.FreeCamera)
    {
        return;
    }

    Engine::TransformComponent& cameraTransformComp = cameraEntity.GetComponent<Engine::TransformComponent>();

    // Follow the first player we find
    bool updated = false;
    scene->ForEach<Engine::TransformComponent , CharacterControllerComponent>(
        [&](Engine::Entity /*playerEntity*/,
            Engine::TransformComponent& playerTransformComp,
            CharacterControllerComponent& /*controllerComp*/)
        {
            if (updated) return; // only one

            const glm::vec3 playerPos = playerTransformComp.Translation;
            const float followLerp = 5.0f;

            cameraTransformComp.Translation.x =
                glm::mix(cameraTransformComp.Translation.x, playerPos.x, followLerp * deltaTime);
            cameraTransformComp.Translation.y =
                glm::mix(cameraTransformComp.Translation.y, playerPos.y, followLerp * deltaTime);

            updated = true;
        });
}
