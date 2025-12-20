#include "NpcAIStateSystem.h"
#include <Engine/Scene/Components/NPC/NpcAIStateComponent.h>
#include <Engine/Scene/Components/NPC/NpcAIComponent.h>
#include <Engine/Scene/Components/Animation/NpcAnimationControllerComponent.h>
#include "NpcAIMovementSystem.h"

void NpcAIStateSystem::UpdateNpcAIStateSystem(float dt, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    scene->ForEach<NpcAIStateComponent, NPCAIMovementComponent, NPCAIVisionComponent,
        Engine::TransformComponent, NpcAnimationControllerComponent, NpcAIPatrolComponent>(
            [&](Engine::Entity, NpcAIStateComponent& npcStateComp, NPCAIMovementComponent& movementComp, NPCAIVisionComponent& visionComp,
                Engine::TransformComponent& tr, NpcAnimationControllerComponent& animCtrl,  NpcAIPatrolComponent& patrol)
            {
                // ---- Clear movement orders each tick ----
                movementComp.wantsMove = 0;
                movementComp.usePath = 0;
                // mv.goal2D left as-is unless we set it

                const bool hasLOS = visionComp.hasLOS;
                const float dist = visionComp.distToTarget;

                const float attackRange = 0.50f;
                const float memorySeconds = 50.0f;
                const bool hasLastKnown = (visionComp.lastSeenTarget) /* or != entt::null */;
                const bool memoryFresh = (visionComp.timeSinceSeen < memorySeconds);

                // ---- If in Attack, wait for controller ----
                if (npcStateComp.state == AIState::Attack)
                {
                    if (dist > attackRange)
                    {
                        //stop attacking
                        npcStateComp.state = AIState::ChaseLOS;
                    }
                    if (animCtrl.actionTimer > 0.0f)
                        return;

                    // Return after one-shot
                    npcStateComp.state = hasLOS ? AIState::ChaseLOS : (memoryFresh ? AIState::MoveToLastKnown : AIState::Idle);
                }

                switch (npcStateComp.state)
                {
                case AIState::Idle:
                {
                    if (hasLOS)
                    {
                        npcStateComp.state = AIState::ChaseLOS;
                        break;
                    }

                    npcStateComp.idleTimer += dt;
                    if (npcStateComp.idleTimer >= npcStateComp.idleDuration)
                    {
                        npcStateComp.idleTimer = 0.0f;
                        if (!patrol.points.empty())
                            npcStateComp.state = AIState::Patrol;
                    }
                    break;
                }

                case AIState::Patrol:
                {
                    if (hasLOS)
                    {
                        npcStateComp.state = AIState::ChaseLOS;
                        break;
                    }

                    if (patrol.points.empty())
                    {
                        npcStateComp.state = AIState::Idle;
                        break;
                    }

                    glm::vec3 target3 = patrol.points[patrol.index];
                    movementComp.wantsMove = 1;
                    movementComp.usePath = 0;
                    movementComp.goal2D = { target3.x, target3.y };

                    // Arrive check
                    glm::vec2 p(tr.Translation.x, tr.Translation.y);
                    glm::vec2 g(target3.x, target3.y);
                    const float reach = 0.10f;
                    if (glm::dot(g - p, g - p) <= reach * reach)
                    {
                        patrol.index = (patrol.index + 1u) % (uint32_t)patrol.points.size();
                        npcStateComp.state = AIState::Idle;
                    }
                    break;
                }

                case AIState::ChaseLOS:
                {
                    if (!hasLOS)
                    {
                        npcStateComp.state = (hasLastKnown && memoryFresh) ? AIState::MoveToLastKnown : AIState::Idle;
                        break;
                    }

                    if (dist <= attackRange && hasLOS)
                    {
                        npcStateComp.state = AIState::Attack;
                        
                        break;
                    }

                    // chase current seen target position
                    movementComp.wantsMove = 1;
                    movementComp.usePath = 0;
                    movementComp.goal2D = { visionComp.lastSeenPos.x, visionComp.lastSeenPos.y }; // current player pos when hasLOS
                    break;
                }

                case AIState::MoveToLastKnown:
                {
                    if (hasLOS)
                    {
                        npcStateComp.state = AIState::ChaseLOS;
                        break;
                    }

                    if (!hasLastKnown || !memoryFresh)
                    {
                        npcStateComp.state = AIState::Idle;
                        break;
                    }

                    // path to last seen position
                    movementComp.wantsMove = 1;
                    movementComp.usePath = 1;
                    movementComp.goal2D = { visionComp.lastSeenPos.x, visionComp.lastSeenPos.y };

                    // If close enough to last seen, give up
                    glm::vec2 p(tr.Translation.x, tr.Translation.y);
                    glm::vec2 g(visionComp.lastSeenPos.x, visionComp.lastSeenPos.y);
                    const float reach = 0.20f;
                    if (glm::dot(g - p, g - p) <= reach * reach)
                    {
                        npcStateComp.state = AIState::Idle;
                    }
                    break;
                }

                default:
                    break;
                }
            });
}

