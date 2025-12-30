#include "ThrowableSystem.h"
#include <Engine/Scene/Components/Combat/ThrowableComponent.h>
#include <Engine/Core/Core.h>
#include <Engine/Map/Grid/GridMap.h>
#include <Engine/Scene/Scene.h>
#include <random>



void ThrowableSystem::UpdateThrowableSystem(float dt, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    // 0) Derive isoUpDirWS from camera (like we discussed before)
    glm::vec2 isoUpDirWS(0.0f, 1.0f);
    scene->ForEach<Engine::TransformComponent, Engine::CameraComponent>(
        [&](Engine::Entity /*camE*/, Engine::TransformComponent& camTr, Engine::CameraComponent& camComp)
        {
            if (!camComp.Primary)
                return;

            const glm::mat4 view = camComp.Camera.GetView();
            glm::vec3 camUpWorld(view[0][1], view[1][1], view[2][1]);
            glm::vec2 up2D(camUpWorld.x, camUpWorld.y);
            if (glm::length2(up2D) > 0.0001f)
                isoUpDirWS = glm::normalize(up2D);
        });

    std::vector<Engine::Entity> toDestroy;
    toDestroy.reserve(16);

    scene->ForEach<Engine::TransformComponent, ThrowableComponent>(
        [&](Engine::Entity e,
            Engine::TransformComponent& tr,
            ThrowableComponent& g)
        {
            if (g.AngularSpeedZ == 0.0f)
            {
                float sign = (RandomRange(0.0f, 1.0f) < 0.5f) ? -1.0f : 1.0f;
                g.AngularSpeedZ = sign * RandomRange(g.MinSpinSpeed, g.MaxSpinSpeed);
            }

            g.FuseTimer -= dt;
            if (g.FuseTimer <= 0.0f && !g.Triggered)
            {
                tr.Translation.x = g.GroundPosWS.x;
                tr.Translation.y = g.GroundPosWS.y;

                TriggerThrowableExplosion(scene, tr, g);
                g.Triggered = true;
                toDestroy.push_back(e);
                return;
            }
            if (g.Triggered)
            {
                toDestroy.push_back(e);
                return;
            }

            // Arc progress with mini ground bounces
            if (g.ArcDuration > 0.0f)
            {
                g.ArcT += dt / g.ArcDuration;

                if (g.ArcT >= 1.0f)
                {
                    g.ArcT = 1.0f;

                    const float speedSq = glm::length2(g.VelocityWS);
                    const float minHopSpeedSq = 0.5f * 0.5f;

                    if (g.GroundBounceCount < g.MaxGroundBounces &&
                        speedSq > minHopSpeedSq)
                    {
                        // start very small extra hop
                        g.GroundBounceCount++;

                        g.ArcT = 0.0f;
                        g.ArcDuration *= 0.4f;
                        g.ArcHeightWorld *= 0.3f;

                        // Change spin on ground bounce
                        float sign = (RandomRange(0.0f, 1.0f) < 0.5f) ? -1.0f : 1.0f;
                        g.AngularSpeedZ = sign * RandomRange(g.MinSpinSpeed, g.MaxSpinSpeed);
                    }
                    else
                    {
                        g.ArcDuration = 0.0f;
                        g.ArcHeightWorld = 0.0f;
                    }
                }
            }
            // Air drag on horizontal velocity
            if (g.AirDrag > 0.0f)
            {
                float speedSq = glm::length2(g.VelocityWS);
                if (speedSq > 0.0f)
                {
                    float dragFactor = glm::max(0.0f, 1.0f - g.AirDrag * dt);
                    g.VelocityWS *= dragFactor;
                }
            }

            // Resting check: if basically stopped and arc is done, freeze it ---
            {
                const float stopSpeedSq = g.MinSpeedToStop * g.MinSpeedToStop;
                float speedSqNow = glm::length2(g.VelocityWS);

                // ArcDuration <= 0 -> no more hops; we want a fully resting state
                if (g.ArcDuration <= 0.0f && speedSqNow < stopSpeedSq)
                {
                    g.VelocityWS = glm::vec2(0.0f);
                    g.AngularSpeedZ = 0.0f;

                    // Base world position stays at last ground position
                    glm::vec2 basePos = g.GroundPosWS;

                    // ArcT is irrelevant now but we keep the same logic for safety
                    float t = glm::clamp(g.ArcT, 0.0f, 1.0f);
                    float s = std::sin(glm::pi<float>() * t);
                    float arcShape = s * s;

                    float height = glm::clamp(g.ArcHeightWorld, 0.3f, 1.5f);
                    float hopScale = (g.GroundBounceCount > 0) ? 0.2f : 1.0f;
                    float totalOffsetAmount = g.InitialLift + arcShape * height * hopScale;

                    glm::vec2 arcOffset = isoUpDirWS * totalOffsetAmount;
                    glm::vec2 renderPos = basePos + arcOffset;

                    tr.Translation.x = renderPos.x;
                    tr.Translation.y = renderPos.y;
                    tr.Translation.z = 0.0f;

                    // Keep whatever final rotation we ended up with
                    tr.Rotation.z = g.RotationZ;

                    // We’re resting, so skip bounce & spin integration
                    return;
                }
            }

            glm::vec2 oldPos = g.GroundPosWS;
            glm::vec2 newPos = oldPos + g.VelocityWS * dt;

            BounceAgainstWorld(scene, oldPos, newPos, g);
            g.GroundPosWS = newPos;

            glm::vec2 basePos = g.GroundPosWS;

            float t = glm::clamp(g.ArcT, 0.0f, 1.0f);

            float s = std::sin(glm::pi<float>() * t);
            float arcShape = s * s;

            float height = g.ArcHeightWorld;

            height = glm::clamp(height, 0.3f, 1.5f);

            float hopScale = (g.GroundBounceCount > 0) ? 0.2f : 1.0f;

            // Final vertical amount along isoUpDirWS
            float totalOffsetAmount = g.InitialLift + arcShape * height * hopScale;

            glm::vec2 arcOffset = isoUpDirWS * totalOffsetAmount;

            glm::vec2 renderPos = basePos + arcOffset;

            tr.Translation.x = renderPos.x;
            tr.Translation.y = renderPos.y;
            tr.Translation.z = 0.0f;

            {
                const float stopSpeedSq = g.MinSpeedToStop * g.MinSpeedToStop;
                float speedSqNow = glm::length2(g.VelocityWS);

                if (speedSqNow >= stopSpeedSq)
                {
                    g.RotationZ += g.AngularSpeedZ * dt;

                    if (g.RotationZ > glm::two_pi<float>())
                        g.RotationZ -= glm::two_pi<float>();
                    else if (g.RotationZ < -glm::two_pi<float>())
                        g.RotationZ += glm::two_pi<float>();
                }

                tr.Rotation.z = g.RotationZ;
            }

            // 8) Stop if slow (for future frames)
            {
                const float stopSpeedSq = g.MinSpeedToStop * g.MinSpeedToStop;
                if (glm::length2(g.VelocityWS) < stopSpeedSq)
                {
                    g.VelocityWS = glm::vec2(0.0f);
                    g.AngularSpeedZ = 0.0f;
                }
            }

        });

    for (Engine::Entity e : toDestroy)
        scene->DestroyEntity(e);
}




void ThrowableSystem::TriggerThrowableExplosion(Engine::Scene* scene, const Engine::TransformComponent& tr,
    const ThrowableComponent& g)
{
    const glm::vec2 pos2D(tr.Translation.x, tr.Translation.y);
    EE_INFO("explosion");
    // submit Explosion?
}

void ThrowableSystem::BounceAgainstWorld(Engine::Scene* scene,
    const glm::vec2& oldPos,
    glm::vec2& newPos,
    ThrowableComponent& g)
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
    const float bounceScale = g.Bounciness * impactDamping;

    // Reflect velocity around surface normal and apply bounceScale
    // glm::reflect(I, N) = I - 2 * dot(N, I) * N
    g.VelocityWS = glm::reflect(g.VelocityWS, normal) * bounceScale;

    g.BounceCount++;

    if (g.ExplodeOnImpact)
    {
        // simplest: arm/trigger explosion by killing fuse
        g.FuseTimer = 0.0f;
    }

    if (g.BounceCount >= g.MaxBounces)
    {
        // After too many bounces, stop it
        g.VelocityWS = glm::vec2(0.0f);
    }
    float sign = (RandomRange(0.0f, 1.0f) < 0.5f) ? -1.0f : 1.0f;
    g.AngularSpeedZ = sign * RandomRange(g.MinSpinSpeed, g.MaxSpinSpeed);

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

