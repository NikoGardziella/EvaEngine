#include "NpcBodyStateSystem.h"
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Scene.h>

#include <Engine/Scene/Components/NPC/NpcBodyStateComponent.h>
#include <Engine/Scene/Components/NPC/Destruction/EnemyDestructibleComponent.h>
#include "Utils/NPCStateUtils.h"


    


void NpcBodyStateSystem::UpdateNpcBodyStateSystem(float deltatime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    scene->ForEach<Engine::EnemyDestructibleComponent, NpcBodyStateComponent>(
        [&](Engine::Entity, Engine::EnemyDestructibleComponent& destr, NpcBodyStateComponent& body)
        {
            bool hasLeftLeg = false;
            bool hasRightLeg = false;
            bool hasAnyArm = false;
            bool hasTorso = false;

            for (const auto& p : destr.pieces)
            {
                if (!NPCStateUtils::PiecePresent(p))
                    continue;

                if (NPCStateUtils::IsTorsoPiece(p.type)) hasTorso = true;
                if (NPCStateUtils::IsLeftLegPiece(p.type))  hasLeftLeg = true;
                if (NPCStateUtils::IsRightLegPiece(p.type)) hasRightLeg = true;
                if (NPCStateUtils::IsArmPiece(p.type))      hasAnyArm = true;
            }

            body.caps = (uint32_t)Cap_Sense;
            body.moveSpeedMul = 1.0f;
            body.attackRangeMul = 1.0f;

            if (!hasTorso)
            {
                body.locomotion = NpcLocomotion::Dead;
                // no Cap_Move, no attacks
                body.moveSpeedMul = 0.0f;
                return;
            }

            // ---- Locomotion ----
            const bool hasAnyLeg = (hasLeftLeg || hasRightLeg);

            body.caps |= (uint32_t)Cap_Move;

            if (!hasAnyLeg)
            {
                body.locomotion = NpcLocomotion::Crawl;
                body.moveSpeedMul = 0.35f;
            }
            else
            {
                body.locomotion = NpcLocomotion::Walk;

                // Limp if only one leg
                 if (hasLeftLeg ^ hasRightLeg)
                    body.moveSpeedMul = 0.60f;
                else
                    body.moveSpeedMul = 1.0f;
            }

            // ---- Attack capability ----
            if (hasAnyArm)
            {
                body.caps |= (uint32_t)Cap_AttackMelee;
                body.attackRangeMul = 1.0f;
            }
            else
            {
                // Either fallback attack (bite) or disable attacks entirely.
                body.caps |= (uint32_t)Cap_AttackBite;
                body.attackRangeMul = 0.75f;
            }

            body.canAttack = hasAnyArm ? 1 : 0;

            // Detect falling condition: had legs before, now none
            if (body.prevHadAnyLeg && !hasAnyLeg)
            {
                // Only trigger if not dead
                if (body.locomotion != NpcLocomotion::Dead)
                {
                    body.transition = NpcTransition::FallToProne;
                    body.locomotion = NpcLocomotion::Prone; // enter prone immediately; animation will play the fall
                }
            }

            body.prevHadAnyLeg = hasAnyLeg ? 1 : 0;


        });
}
