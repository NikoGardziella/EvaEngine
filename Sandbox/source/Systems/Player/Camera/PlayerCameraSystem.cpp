#include "PlayerCameraSystem.h"
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>

void PlayerCameraSystem::UpdatePlayerCameraSystem(entt::registry& registry, float deltaTime, Engine::Scene* scene)
{
    // Find player (first entity with CharacterControllerComponent)
    auto view = registry.view<Engine::TransformComponent, CharacterControllerComponent>();

    for (auto entity : view)
    {
        auto& playerTransform = view.get<Engine::TransformComponent>(entity);

        entt::entity cameraEntity = scene->GetPrimaryCameraEntity();
        if (cameraEntity == entt::null)
            return;

        auto& cameraTransform = registry.get<Engine::TransformComponent>(cameraEntity);

        glm::vec3 playerPos = playerTransform.Translation;
        cameraTransform.Translation.x = glm::mix(cameraTransform.Translation.x, playerPos.x, 5.0f * deltaTime);
        cameraTransform.Translation.y = glm::mix(cameraTransform.Translation.y, playerPos.y, 5.0f * deltaTime);

    }
   
}