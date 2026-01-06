#include "WeaponSpawnSystem.h"
#include <Engine/Scene/Components/Combat/EquippedWeaponComponent.h>
#include <Engine/Scene/Components/Render/3D/SkeletonComponent.h>
#include "Engine/Scene/Entity.h"
#include <Engine/Scene/Components/Render/3D/MeshRefComponent.h>
#include <Engine/Scene/Components/Combat/ThrowableComponent.h>
#include "Engine/AssetManager/AssetManager.h"
#include <Engine/Scene/Components/Render/3D/RenderBoundsComponent.h>

void WeaponSpawnSystem::UpdateWeaponSpawnSystem(float dt, Engine::Scene* scene)
{
    scene->ForEach<WeaponInventoryComponent, Engine::TransformComponent, Engine::SkeletonComponent>(
        [&](Engine::Entity player, WeaponInventoryComponent& weaponInventoryComp, Engine::TransformComponent& transformComp, Engine::SkeletonComponent& skeletonComp)
        {
            if (weaponInventoryComp.equipDirty)
            {
                weaponInventoryComp.equipDirty = false;

                if (weaponInventoryComp.equippedWeaponEntity)
                    scene->DestroyEntity(weaponInventoryComp.equippedWeaponEntity);


                Engine::MeshRegistry& meshReg = Engine::AssetManager::GetMeshRegistry();

                const Engine::MeshAsset* meshAsset = meshReg.GetMeshByKey("ak47");



                Engine::Entity equippedWeaponEntity = scene->CreateEntity("Weapon_Equipped");
                weaponInventoryComp.equippedWeaponEntity = equippedWeaponEntity;

                Engine::RenderBoundsComponent& rbComp = equippedWeaponEntity.AddComponent<Engine::RenderBoundsComponent>();
                rbComp.maxL = meshAsset->maxL;
                rbComp.minL = meshAsset->minL;


                Engine::TransformComponent& weaponTransformComp = equippedWeaponEntity.AddComponent<Engine::TransformComponent>();
                weaponTransformComp.Translation = transformComp.Translation; // temporary




                Engine::MeshRefComponent& MeshRefComp = equippedWeaponEntity.AddComponent<Engine::MeshRefComponent>();
                MeshRefComp.meshId = meshAsset->id;
                MeshRefComp.submeshCount = meshAsset->submeshes.size();
                MeshRefComp.submeshFirst = 0;

                const Engine::SkeletonAsset& skelAsset =
                    Engine::AssetManager::GetSkeletonRegistry().Get(skeletonComp.skeletonId);

                // Try common right-hand names
                uint32_t bone = Engine::SkeletonRegistry::FindBoneContains(skelAsset, "RightHand");
                if (bone == 0xFFFFFFFFu) bone = Engine::SkeletonRegistry::FindBoneContains(skelAsset, "hand_right");
                if (bone == 0xFFFFFFFFu) bone = Engine::SkeletonRegistry::FindBoneContains(skelAsset, "mixamorig:RightHand");

                // Fallback
                if (bone == 0xFFFFFFFFu)
                {
                    EE_CORE_WARN("Right hand bone not found, defaulting to root");
                    bone = 0; // root
                }

                glm::mat4 flipX = glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(1, 0, 0));

                glm::mat4 localOffset = glm::translate(glm::mat4(1.0f), glm::vec3(-0.07f, 0.1f, 0.0f)) * flipX;


                EquippedWeaponComponent& equippedWeaponComp = equippedWeaponEntity.AddComponent<EquippedWeaponComponent>();
                equippedWeaponComp.owner = player;
                equippedWeaponComp.attachBoneIndex = bone;
                equippedWeaponComp.localOffset = localOffset;
                equippedWeaponComp.visible = true;
            }

            

            
        });
}
