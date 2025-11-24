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

namespace Engine {


    void RenderSystem3D::Render(const VisibleSet& vis,Scene* scene,
        const TransformSystem3D& xforms, const MeshRegistry& meshes, const MaterialRegistry& materials)
    {
        EE_PROFILE_FUNCTION();

        for (Entity e : vis.entities)
        {
            const glm::mat4* pWorldTransform = xforms.TryGetWorld(e);
            if (!pWorldTransform) continue;

            // Static meshes
            if (auto mr = scene->TryGet<MeshRefComponent>(e))
            {
                InstanceDataGPU inst{};
                inst.world = *pWorldTransform;
                inst.worldPrev = *pWorldTransform;
                inst.boneBase = 0xFFFFFFFFu;
                inst.flags = 0;
                inst.objectId = /* some id */ 0;
                inst.meshId = mr->meshId;

                if (auto mat = scene->TryGet<MaterialRefComponent>(e))
                    inst.materialId = mat->materialId;
                else
                    inst.materialId = 0;

                // Submit whole submesh range
                VulkanRenderer3D::SubmitMeshInstanceRange(inst, mr->submeshFirst, mr->submeshCount);
            }

            // Skinned meshes
            if (auto smr = scene->TryGet<SkinnedMeshRefComponent>(e))
            {
                InstanceDataGPU inst{};
                inst.world = *pWorldTransform;
                inst.worldPrev = *pWorldTransform;
                inst.flags = 0;
                inst.objectId = /* your id */ 0;

                if (auto mat = scene->TryGet<MaterialRefComponent>(e))
                    inst.materialId = mat->materialId;
                else
                    inst.materialId = 0;

                if (auto sk = scene->TryGet<SkeletonComponent>(e))
                    inst.boneBase = sk->boneBase;
                else
                    inst.boneBase = 0xFFFFFFFFu; // guard

                

                VulkanRenderer3D::SubmitMeshInstanceRange(inst, smr->submeshFirst, smr->submeshCount);
            }
        }
    }


} 
