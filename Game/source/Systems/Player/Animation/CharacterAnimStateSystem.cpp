#include "CharacterAnimStateSystem.h"
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>
#include <Engine/Scene/Components/Combat/WeaponComponent.h>
#include <Engine/Scene/Components/Animation/PlayerAnimation/CharacterAnimStateComponent.h>
#include "Engine/Scene/Entity.h"

void CharacterAnimStateSystem::UpdateCharacterAnimStateSystem(float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    scene->ForEach<Engine::TransformComponent, CharacterControllerComponent, WeaponComponent, CharacterAnimStateComponent>(
        [&](Engine::Entity /*e*/, Engine::TransformComponent& tr, CharacterControllerComponent& cc, WeaponComponent& weap,
            CharacterAnimStateComponent& st)
        {
            const float speed = glm::length(glm::vec2(cc.velocity.x, cc.velocity.y));

            st.locomotion = (speed < 0.05f) ? LocomotionState::Idle : LocomotionState::Run;

            st.aiming = weap.IsAiming;

            st.firing = weap.IsFiring;
           // weap.FiredThisFrame = false;
        });
}
