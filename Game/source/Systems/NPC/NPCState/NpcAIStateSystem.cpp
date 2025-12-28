#include "NpcAIStateSystem.h"
#include <Engine/Scene/Components/NPC/NpcAIStateComponent.h>
#include <Engine/Scene/Components/NPC/NpcAIComponent.h>
#include <Engine/Scene/Components/Animation/NpcAnimationControllerComponent.h>
#include <random>
#include <Engine/Scene/Components/Combat/HealthComponent.h>

void NpcAIStateSystem::UpdateNpcAIStateSystem(float dt, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    scene->ForEach<NpcAIStateComponent, NPCAIMovementComponent, NPCAIVisionComponent,
        Engine::TransformComponent, NpcAnimationControllerComponent, NpcAIPatrolComponent, HealthComponent>(
            [&](Engine::Entity, NpcAIStateComponent& npcStateComp, NPCAIMovementComponent& movementComp, NPCAIVisionComponent& visionComp,
                Engine::TransformComponent& transformComp, NpcAnimationControllerComponent& animCtrlComp,  NpcAIPatrolComponent& patrolComp, HealthComponent& healthComp)
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


                if (healthComp.Current <= 0.0f)
                {
                    npcStateComp.state = AIState::Dead;
                    return;
                }

                // ---- If in Attack, wait for controller ----
                if (npcStateComp.state == AIState::Attack)
                {
                    if (dist > attackRange)
                    {
                        //stop attacking and chase
                        npcStateComp.state = AIState::ChaseLOS;
                    }

                    if (hasLOS)
                    {
                        npcStateComp.state = AIState::ChaseLOS;
                    }
                    else if (memoryFresh)
                    {
                        npcStateComp.state = AIState::MoveToLastKnown;
                    }
                    else
                    {
                        npcStateComp.state = AIState::Idle;
                    }
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

                    // If we are close to the current goal, pick a new random one
                    npcStateComp.patrolRegoalCooldown -= dt;

                    glm::vec2 p(transformComp.Translation.x, transformComp.Translation.y);
                    glm::vec2 g = movementComp.goal2D;

                    const float reach = 0.35f;
                    const bool arrived = glm::dot(g - p, g - p) <= reach * reach;

                    if (arrived && npcStateComp.patrolRegoalCooldown <= 0.0f)
                    {
                        SetRandomPatrolGoal(movementComp, transformComp.Translation, 2.0f, 10.0f, /*usePath=*/true);
                        npcStateComp.patrolRegoalCooldown = 0.75f; // don’t repick instantly
                    }
                    else
                    {
                        // keep moving to current goal
                        movementComp.wantsMove = 1;
                        movementComp.usePath = 1;
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
                    glm::vec2 p(transformComp.Translation.x, transformComp.Translation.y);
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


void NpcAIStateSystem::SetRandomPatrolGoal(NPCAIMovementComponent& movementComp, const glm::vec3& npcPos, float minRadius,
    float maxRadius, bool usePath = true)
{
    // One RNG for the whole program/thread (fine for spawning / AI)
    static thread_local std::mt19937 rng{ std::random_device{}() };

    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318530718f);
    std::uniform_real_distribution<float> tDist(0.0f, 1.0f);

    const float a = angleDist(rng);

    // Uniform area in annulus [minR, maxR]
    const float t = tDist(rng);
    const float r = std::sqrt((1.0f - t) * (minRadius * minRadius) + t * (maxRadius * maxRadius));

    const glm::vec2 center(npcPos.x, npcPos.y);
    const glm::vec2 dir(std::cos(a), std::sin(a));
    const glm::vec2 goal = center + dir * r;

    movementComp.goal2D = goal;
    movementComp.wantsMove = 1;
    movementComp.usePath = usePath ? 1 : 0;
}
