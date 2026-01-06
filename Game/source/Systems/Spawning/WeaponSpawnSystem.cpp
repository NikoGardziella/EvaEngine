#include "WeaponSpawnSystem.h"
#include <Engine/Scene/Components/Combat/EquippedWeaponComponent.h>
#include <Engine/Scene/Components/Render/3D/SkeletonComponent.h>
#include "Engine/Scene/Entity.h"
#include <Engine/Scene/Components/Render/3D/MeshRefComponent.h>
#include <Engine/Scene/Components/Combat/ThrowableComponent.h>
#include "Engine/AssetManager/AssetManager.h"
#include <Engine/Scene/Components/Render/3D/RenderBoundsComponent.h>
#include <Engine/Scene/Components/Combat/WeaponDef.h>
void WeaponSpawnSystem::UpdateWeaponSpawnSystem(float dt, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    scene->ForEach<WeaponInventoryComponent, Engine::TransformComponent, Engine::SkeletonComponent>(
        [&](Engine::Entity playerEntity, WeaponInventoryComponent& weaponInventoryComp, Engine::TransformComponent& playerTransformComp, Engine::SkeletonComponent& skelComp)
        {
            if (!weaponInventoryComp.equipDirty)
                return;

            weaponInventoryComp.equipDirty = false;

            // Resolve desired WeaponType from slot
            WeaponType desiredType = WeaponType::MachineGun;
            switch (weaponInventoryComp.desiredSlot)
            {
            case WeaponSlot::Slot1: desiredType = weaponInventoryComp.slot1; break;
            case WeaponSlot::Slot2: desiredType = weaponInventoryComp.slot2; break;
            case WeaponSlot::Slot3: desiredType = weaponInventoryComp.slot3; break;
            default: desiredType = weaponInventoryComp.slot1; break;
            }

            // Destroy old equipped entity
            if (weaponInventoryComp.equippedWeaponEntity)
            {
                scene->DestroyEntity(weaponInventoryComp.equippedWeaponEntity);
                weaponInventoryComp.equippedWeaponEntity = {};
            }

            // Build definition
            const WeaponDefinition::WeaponDef def = WeaponDefinition::MakeWeaponDef(desiredType);


            Engine::MeshRegistry& meshReg = Engine::AssetManager::GetMeshRegistry();
            const Engine::MeshAsset* meshAsset = meshReg.GetMeshByKey(def.meshKey);
            if (!meshAsset)
            {
                EE_CORE_ERROR("Weapon mesh not found: {}", def.meshKey ? def.meshKey : "<null>");
                weaponInventoryComp.equippedSlot = weaponInventoryComp.desiredSlot;
                return;
            }

            Engine::Entity weaponE = scene->CreateEntity("Weapon_Equipped");
            weaponInventoryComp.equippedWeaponEntity = weaponE;
            weaponInventoryComp.equippedSlot = weaponInventoryComp.desiredSlot;

            // bounds
            auto& rb = weaponE.AddComponent<Engine::RenderBoundsComponent>();
            rb.maxL = meshAsset->maxL;
            rb.minL = meshAsset->minL;

            auto& weaponTr = weaponE.AddComponent<Engine::TransformComponent>();
            weaponTr.Translation = playerTransformComp.Translation;

            // mesh ref
            auto& mref = weaponE.AddComponent<Engine::MeshRefComponent>();
            mref.meshId = meshAsset->id;
            mref.submeshCount = (uint32_t)meshAsset->submeshes.size();
            mref.submeshFirst = 0;

            // remove old component and make new one
            if (playerEntity.HasComponent<WeaponComponent>())
            {
                playerEntity.RemoveComponent<WeaponComponent>();
            }
            WeaponComponent& weap = playerEntity.AddComponent<WeaponComponent>(def.weaponStats);

            // find bone
            const Engine::SkeletonAsset& skelAsset =  Engine::AssetManager::GetSkeletonRegistry().Get(skelComp.skeletonId);

            uint32_t bone = Engine::SkeletonRegistry::FindBoneContains(skelAsset, "RightHand");
            if (bone == 0xFFFFFFFFu) bone = Engine::SkeletonRegistry::FindBoneContains(skelAsset, "hand_right");
            if (bone == 0xFFFFFFFFu) bone = Engine::SkeletonRegistry::FindBoneContains(skelAsset, "mixamorig:RightHand");

            if (bone == 0xFFFFFFFFu)
            {
                EE_CORE_WARN("Right hand bone not found, defaulting to root");
                bone = 0;
            }

            // equipped/attach
            auto& eq = weaponE.AddComponent<EquippedWeaponComponent>();
            eq.owner = playerEntity;
            eq.attachBoneIndex = bone;
            eq.localOffset = def.localOffset;
            eq.visible = true;
        });
}
