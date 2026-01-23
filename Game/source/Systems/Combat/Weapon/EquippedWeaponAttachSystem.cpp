#include "EquippedWeaponAttachSystem.h"
#include <Engine/Scene/Components/Combat/EquippedWeaponComponent.h>
#include <Engine/Scene/Components/Render/3D/AnimatorComponent.h>
#include <Engine/Scene/Component.h>

void EquippedWeaponAttachSystem::UpdateEquippedWeaponAttachSystem(float dt, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    scene->ForEach<EquippedWeaponComponent, Engine::TransformComponent>(
        [&](Engine::Entity weapon, EquippedWeaponComponent& equippedWeaponCOmp, Engine::TransformComponent& transformComp)
        {
            if (!equippedWeaponCOmp.visible) return;

            Engine::Entity player = equippedWeaponCOmp.owner;

            auto* playerTr = scene->TryGet<Engine::TransformComponent>(player);
            auto* anim = scene->TryGet<Engine::Animator3DComponent>(player);
            if (!playerTr || !anim) return;

            const glm::mat4 playerWorld = playerTr->GetTransform();

            const glm::mat4 boneModel = anim->boneModel[equippedWeaponCOmp.attachBoneIndex];

            
         

            glm::mat4 weaponWorld = playerWorld * boneModel * equippedWeaponCOmp.localOffset;

            transformComp.SetTransform(weaponWorld);

        });
}
