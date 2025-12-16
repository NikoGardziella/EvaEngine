#include "pch.h"
#include "RenderSystem3D.h"
#include "TransformSystem3D.h"
#include <Engine/Scene/Components/Render/3D/MeshRefComponent.h>
#include <Engine/Scene/Components/Render/3D/MaterialRefComponent.h>
#include <Engine/Scene/Components/Render/3D/SkinnedMeshRefComponent.h>
#include <Engine/Scene/Components/Render/3D/SkeletonComponent.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Renderer/3D/VulkanRenderer3D.h>
#include "Engine/Animation/3D/VisibleSet.h"
#include <Engine/Scene/Component.h>
#include "Engine/Animation/3D/System/Render3DUtils/Render3DUtils.h"
#include <Engine/Scene/Components/NPC/Destruction/EnemyDestructibleComponent.h>

namespace Engine {


    void RenderSystem3D::Render(const VisibleSet& vis,Scene* scene,
        const TransformSystem3D& xforms, const MeshRegistry& meshes, const MaterialRegistry& materials)
    {
        EE_PROFILE_FUNCTION();

        for (Entity entity : vis.entities)
        {
            const glm::mat4* pWorldTransform = xforms.TryGetWorld(entity);
            if (!pWorldTransform) continue;

            const TransformComponent& transformComp = entity.GetComponent<TransformComponent>();
            // Static meshes
            if (auto mr = scene->TryGet<MeshRefComponent>(entity))
            {
                uint32_t bonebase = 0;

                const SkeletonComponent& skeletonComponent = entity.GetComponent<SkeletonComponent>();
            
                bonebase = skeletonComponent.boneBase;

                InstanceDataGPU inst{};



                inst.world = *pWorldTransform;
               // inst.worldPrev = *pWorldTransform;
                inst.boneBase = bonebase;
                inst.boneCount= skeletonComponent.boneCount;
                //inst.meshId = mr->meshId;
                //inst.flags = 0;
                /*
                inst.objectId =  0; // some id

                if (auto mat = scene->TryGet<MaterialRefComponent>(e))
                    inst.materialId = mat->materialId;
                else
                    inst.materialId = 0;
                */

                // Submit whole submesh range
                if (EnemyDestructibleComponent* destr = entity.TryGetComponent<EnemyDestructibleComponent>())
                {
                    VulkanRenderer3D::SubmitEnemyPieces(inst, mr->meshId, *destr);
                }
                else
                {
                    VulkanRenderer3D::SubmitMeshInstanceRange(inst, mr->meshId, mr->submeshFirst, mr->submeshCount);
                }
            }

            // Skinned meshes
            if (auto smr = scene->TryGet<SkinnedMeshRefComponent>(entity))
            {
                InstanceDataGPU inst{};
                inst.world = *pWorldTransform;
                //inst.meshId = smr->meshId;
                //inst.worldPrev = *pWorldTransform;
               // inst.flags = 0;
               // inst.objectId = /* your id */ 0;

                /*
                if (auto mat = scene->TryGet<MaterialRefComponent>(e))
                    inst.materialId = mat->materialId;
                else
                    inst.materialId = 0;

                */
                if (auto sk = scene->TryGet<SkeletonComponent>(entity))
                    inst.boneBase = sk->boneBase;
                else
                    inst.boneBase = 0xFFFFFFFFu; // guard

                

                VulkanRenderer3D::SubmitMeshInstanceRange(inst, smr->meshId, smr->submeshFirst, smr->submeshCount);
            }
        }
    }


} 
