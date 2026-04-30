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
#include <Engine/Renderer/Renderer2D/VulkanRenderer2D.h>
#include <Engine/Scene/Components/Map/FloorComponent.h>

namespace Engine {


    void RenderSystem3D::Render(const VisibleSet& vis,Scene* scene,
        const TransformSystem3D& xforms, const MeshRegistry& meshes, const MaterialRegistry& materials)
    {
        EE_PROFILE_FUNCTION();

        for (Entity entity : vis.entities)
        {
            const glm::mat4* pWorldTransform = xforms.TryGetWorld(entity);
            if (!pWorldTransform) continue;

            glm::mat4 visualWorld = *pWorldTransform;

            if (const FloorComponent* floor = scene->TryGet<FloorComponent>(entity))
            {
                float visualFloor = float(floor->Floor);

                if (floor->IsChangingFloor)
                {
                    visualFloor = glm::mix(
                        float(floor->Floor),
                        float(floor->TargetFloor),
                        floor->FloorT);
                }

                constexpr float FloorVisualYOffset = TILE_SIZE * 0.5f;

                visualWorld = glm::translate(
                    glm::mat4(1.0f),
                    glm::vec3(0.0f, visualFloor * FloorVisualYOffset, 0.0f)
                ) * visualWorld;
            }


            const TransformComponent& transformComp = entity.GetComponent<TransformComponent>();
            // Static meshes
            if (auto mr = scene->TryGet<MeshRefComponent>(entity))
            {
                uint32_t boneBase = 0xFFFFFFFFu;
                uint32_t boneCount = 0;

                if (SkeletonComponent* sk = scene->TryGet<SkeletonComponent>(entity))
                {
                    boneBase = sk->boneBase;
                    boneCount = sk->boneCount;
                }

                
                InstanceDataGPU inst{};
                inst.boneBase = boneBase;
                inst.boneCount = boneCount;



                inst.world = visualWorld;
               // inst.worldPrev = *pWorldTransform;
               
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
                    bool debugDraw = false;
                    if (debugDraw)
                    {
                        for (size_t i = 0; i < destr->pieces.size(); i++)
                        {
                            DebugDrawHitSphere2D_XY(*pWorldTransform, destr->pieces[i].hitLocalCenter, destr->pieces[i].hitRadius, glm::vec4(1, 0, 0, 1));

                        }
                    }

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
                inst.world = visualWorld;
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


    void RenderSystem3D::DebugDrawHitSphere2D_XY(const glm::mat4& enemyWorld,  const glm::vec3& hitLocalCenter,
        float radius,    const glm::vec4& color)
    {
        // World-space center
        const glm::vec3 c = glm::vec3(enemyWorld * glm::vec4(hitLocalCenter, 1.0f));

        // Crosshair
        {
            glm::vec3 a = glm::vec3(c.x - radius, c.y, 0.1f);
            glm::vec3 b = glm::vec3(c.x + radius, c.y, 0.1f);
            Engine::VulkanRenderer2D::DrawLine(a, b, color, -1);

            a = glm::vec3(c.x, c.y - radius, 0.1f);
            b = glm::vec3(c.x, c.y + radius, 0.1f);
            Engine::VulkanRenderer2D::DrawLine(a, b, color, -1);
        }

    }


} 
