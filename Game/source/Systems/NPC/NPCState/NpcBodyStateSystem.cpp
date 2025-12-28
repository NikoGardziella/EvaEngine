#include "NpcBodyStateSystem.h"
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Scene.h>

#include <Engine/Scene/Components/NPC/NpcBodyStateComponent.h>
#include <Engine/Scene/Components/NPC/Destruction/EnemyDestructibleComponent.h>
#include "Utils/NPCStateUtils.h"
#include <Engine/Scene/Components/Combat/HealthComponent.h>



void NpcBodyStateSystem::UpdateNpcBodyStateSystem(float deltatime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    scene->ForEach<Engine::EnemyDestructibleComponent, NpcBodyStateComponent,HealthComponent>(
        [&](Engine::Entity, Engine::EnemyDestructibleComponent& destrComp, NpcBodyStateComponent& bodyStateComp, HealthComponent& healthComp)
        {
            bool hasLeftLeg = false;
            bool hasRightLeg = false;
            bool hasAnyArm = false;
            bool hasTorso = false;

            for (const auto& p : destrComp.pieces)
            {
                if (!NPCStateUtils::PiecePresent(p))
                    continue;

                if (NPCStateUtils::IsTorsoPiece(p.type)) hasTorso = true;
                if (NPCStateUtils::IsLeftLegPiece(p.type))  hasLeftLeg = true;
                if (NPCStateUtils::IsRightLegPiece(p.type)) hasRightLeg = true;
                if (NPCStateUtils::IsArmPiece(p.type))      hasAnyArm = true;
            }

            bodyStateComp.caps = (uint32_t)Cap_Sense;
            bodyStateComp.moveSpeedMul = 1.0f;
            bodyStateComp.attackRangeMul = 1.0f;

          
            // ---- Locomotion ----
            const bool hasAnyLeg = (hasLeftLeg || hasRightLeg);

            bodyStateComp.caps |= (uint32_t)Cap_Move;

            if (!hasAnyLeg)
            {
                bodyStateComp.locomotion = NpcLocomotion::Crawl;
                bodyStateComp.moveSpeedMul = 0.35f;
            }
            else
            {
                bodyStateComp.locomotion = NpcLocomotion::Walk;

                // Limp if only one leg
                 if (hasLeftLeg ^ hasRightLeg)
                    bodyStateComp.moveSpeedMul = 0.60f;
                else
                    bodyStateComp.moveSpeedMul = 1.0f;
            }

            // ---- Attack capability ----
            if (hasAnyArm)
            {
                bodyStateComp.caps |= (uint32_t)Cap_AttackMelee;
                bodyStateComp.attackRangeMul = 1.0f;
            }
            else
            {
                // Either fallback attack (bite) or disable attacks entirely.
                bodyStateComp.caps |= (uint32_t)Cap_AttackBite;
                bodyStateComp.attackRangeMul = 0.75f;
            }

            bodyStateComp.canAttack = hasAnyArm ? 1 : 0;

            // Detect falling condition: had legs before, now none
            if (bodyStateComp.prevHadAnyLeg && !hasAnyLeg)
            {
                // Only trigger if not dead
                bodyStateComp.transition = NpcTransition::FallToProne;
                bodyStateComp.locomotion = NpcLocomotion::Prone; // enter prone immediately; animation will play the fall
                
            }

            bodyStateComp.prevHadAnyLeg = hasAnyLeg ? 1 : 0;


        });
}
