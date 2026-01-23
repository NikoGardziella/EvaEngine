#include "ThrowableSystem.h"
#include <Engine/Scene/Components/Combat/ThrowableComponent.h>
#include <Engine/Core/Core.h>
#include <Engine/Map/Grid/GridMap.h>
#include <Engine/Scene/Scene.h>
#include <random>
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Renderer/Renderer2D/VulkanRenderer2D.h>


void ThrowableSystem::UpdateThrowableSystem(float dt, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    std::vector<Engine::Entity> toDestroy;
    toDestroy.reserve(32);

    scene->ForEach<Engine::TransformComponent, ThrowableComponent>(
        [&](Engine::Entity e, Engine::TransformComponent& transformComp, ThrowableComponent& throwableComp)
        {
            if (throwableComp.AngularSpeedZ == 0.0f)
            {
                const float sign = (RandomRange(0.0f, 1.0f) < 0.5f) ? -1.0f : 1.0f;
                throwableComp.AngularSpeedZ = sign * RandomRange(throwableComp.MinSpinSpeed, throwableComp.MaxSpinSpeed);
            }

            throwableComp.FuseTimer -= dt;
            if (throwableComp.FuseTimer <= 0.0f && !throwableComp.Triggered)
            {
                // Snap to ground XY before exploding (no arc)
                transformComp.Translation.x = throwableComp.GroundPosWS.x;
                transformComp.Translation.y = throwableComp.GroundPosWS.y;
                transformComp.Translation.z = 0.0f;

                TriggerThrowableExplosion(scene, transformComp, throwableComp);
                throwableComp.Triggered = true;
                toDestroy.push_back(e);
                return;
            }

            if (throwableComp.Triggered)
            {
                toDestroy.push_back(e);
                return;
            }

            const float stopSpeedSq = throwableComp.MinSpeedToStop * throwableComp.MinSpeedToStop;
            const float minHopSpeedSq = 0.5f * 0.5f;

            if (throwableComp.ArcDuration > 0.0f)
            {
                throwableComp.ArcT += dt / throwableComp.ArcDuration;

                if (throwableComp.ArcT >= 1.0f)
                {
                    throwableComp.ArcT = 1.0f;

                    const float speedSq = glm::length2(throwableComp.VelocityWS);
                    const bool canHop =
                        (throwableComp.GroundBounceCount < throwableComp.MaxGroundBounces) &&
                        (speedSq > minHopSpeedSq);

                    if (canHop)
                    {
                        throwableComp.GroundBounceCount++;

                        // Restart a smaller hop
                        throwableComp.ArcT = 0.0f;
                        throwableComp.ArcDuration *= 0.4f;
                        throwableComp.ArcHeightWorld *= 0.3f;

                        // Change spin on hop
                        const float sign = (RandomRange(0.0f, 1.0f) < 0.5f) ? -1.0f : 1.0f;
                        throwableComp.AngularSpeedZ = sign * RandomRange(throwableComp.MinSpinSpeed, throwableComp.MaxSpinSpeed);
                    }
                    else
                    {
                        throwableComp.ArcDuration = 0.0f;
                        throwableComp.ArcHeightWorld = 0.0f;
                    }
                }
            }

            //Air drag (XY velocity)
            if (throwableComp.AirDrag > 0.0f)
            {
                const float speedSq = glm::length2(throwableComp.VelocityWS);
                if (speedSq > 0.0f)
                {
                    const float dragFactor = glm::max(0.0f, 1.0f - throwableComp.AirDrag * dt);
                    throwableComp.VelocityWS *= dragFactor;
                }
            }

            //  Integrate ground motion in XY + collisions
            const glm::vec2 oldPos = throwableComp.GroundPosWS;
            glm::vec2 newPos = oldPos + throwableComp.VelocityWS * dt;

            BounceAgainstWorld(scene, oldPos, newPos, throwableComp);
            throwableComp.GroundPosWS = newPos;

            // 5) Resting logic (freeze XY + stop spin when arc is done)
            const float speedSqNow = glm::length2(throwableComp.VelocityWS);
            const bool arcDone = (throwableComp.ArcDuration <= 0.0f);

            if (arcDone && speedSqNow < stopSpeedSq)
            {
                throwableComp.VelocityWS = glm::vec2(0.0f);
                throwableComp.AngularSpeedZ = 0.0f;

                // Lock transform at ground
                transformComp.Translation.x = throwableComp.GroundPosWS.x;
                transformComp.Translation.y = throwableComp.GroundPosWS.y;
                

                transformComp.Rotation.z = throwableComp.RotationZ;
                return;
            }

            float t = glm::clamp(throwableComp.ArcT, 0.0f, 1.0f);
            const float s = std::sin(glm::pi<float>() * t);
            const float arcShape = s * s;

            float height = glm::clamp(throwableComp.ArcHeightWorld, 0.0f, 10.0f);
            const float hopScale = (throwableComp.GroundBounceCount > 0) ? 0.2f : 1.0f;

            const float z = glm::max(0.0f, throwableComp.InitialLift + arcShape * height * hopScale);

            // 7) Write final render transform (XY from ground, Z from arc)
            transformComp.Translation.x = throwableComp.GroundPosWS.x;
            transformComp.Translation.y = throwableComp.GroundPosWS.y;
            transformComp.Translation.z = z;

            if (speedSqNow >= stopSpeedSq)
            {
                throwableComp.RotationZ += throwableComp.AngularSpeedZ * dt;

                if (throwableComp.RotationZ > glm::two_pi<float>())
                    throwableComp.RotationZ -= glm::two_pi<float>();
                else if (throwableComp.RotationZ < -glm::two_pi<float>())
                    throwableComp.RotationZ += glm::two_pi<float>();
            }

            transformComp.Rotation.z = throwableComp.RotationZ;

            if (glm::length2(throwableComp.VelocityWS) < stopSpeedSq)
            {
                throwableComp.VelocityWS = glm::vec2(0.0f);
                throwableComp.AngularSpeedZ = 0.0f;
            }
        });

    for (Engine::Entity e : toDestroy)
        scene->DestroyEntity(e);
}





void ThrowableSystem::TriggerThrowableExplosion(Engine::Scene* scene, const Engine::TransformComponent& tr, const ThrowableComponent& throwableComp)
{
    const glm::vec2 pos2D(tr.Translation.x, tr.Translation.y);
    Engine::VulkanRenderer2D::SubmitCPUExplosion(pos2D, throwableComp.DestructionRadius, throwableComp.Damge);
}

void ThrowableSystem::BounceAgainstWorld(Engine::Scene* scene, const glm::vec2& oldPos, glm::vec2& newPos, ThrowableComponent& throwableComp)
{
    Engine::Ref<Engine::GridMap> grid = scene->GetGrid();
    if (!grid)
        return;

    glm::vec2 normal;
    if (!IsBlocked(grid, newPos, normal))
        return;

    // Move back to old position (or slightly away along the normal)
    newPos = oldPos + normal * 0.01f; // small push so we don't stay inside

    // ---- Stronger damping per bounce ----
    // Base bounciness from component, plus extra impact damping
    const float impactDamping = 0.5f;  // 0.5 = lose extra 50% of speed on impact
    const float bounceScale = throwableComp.Bounciness * impactDamping;

    // Reflect velocity around surface normal and apply bounceScale
    throwableComp.VelocityWS = glm::reflect(throwableComp.VelocityWS, normal) * bounceScale;

    throwableComp.BounceCount++;

    if (throwableComp.ExplodeOnImpact)
    {
        throwableComp.FuseTimer = 0.0f;
    }

    if (throwableComp.BounceCount >= throwableComp.MaxBounces)
    {
        // After too many bounces, stop it
        throwableComp.VelocityWS = glm::vec2(0.0f);
    }
    float sign = (RandomRange(0.0f, 1.0f) < 0.5f) ? -1.0f : 1.0f;
    throwableComp.AngularSpeedZ = sign * RandomRange(throwableComp.MinSpinSpeed, throwableComp.MaxSpinSpeed);

}

float ThrowableSystem::RandomRange(float minVal, float maxVal)
{
    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> dist(minVal, maxVal);
    return dist(rng);
}

bool ThrowableSystem::IsBlocked(Engine::Ref<Engine::GridMap> grid, const glm::vec2& posWS, glm::vec2& outNormal)
{
    if (!grid)
        return false;

    return grid->IsPointBlockedWithNormal(posWS, outNormal);
}

