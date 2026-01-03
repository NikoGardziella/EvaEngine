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
        [&](Engine::Entity /*camE*/,const Engine::TransformComponent& camTr, const Engine::CameraComponent& camComp)
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
        [&](Engine::Entity e, Engine::TransformComponent& throwableTransformComp, ThrowableComponent& throwableComp)
        {
            if (throwableComp.AngularSpeedZ == 0.0f)
            {
                float sign = (RandomRange(0.0f, 1.0f) < 0.5f) ? -1.0f : 1.0f;
                throwableComp.AngularSpeedZ = sign * RandomRange(throwableComp.MinSpinSpeed, throwableComp.MaxSpinSpeed);
            }

            throwableComp.FuseTimer -= dt;
            if (throwableComp.FuseTimer <= 0.0f && !throwableComp.Triggered)
            {
                throwableTransformComp.Translation.x = throwableComp.GroundPosWS.x;
                throwableTransformComp.Translation.y = throwableComp.GroundPosWS.y;

                TriggerThrowableExplosion(scene, throwableTransformComp, throwableComp);
                throwableComp.Triggered = true;
                toDestroy.push_back(e);
                return;
            }
            if (throwableComp.Triggered)
            {
                toDestroy.push_back(e);
                return;
            }

            // Arc progress with mini ground bounces
            if (throwableComp.ArcDuration > 0.0f)
            {
                throwableComp.ArcT += dt / throwableComp.ArcDuration;

                if (throwableComp.ArcT >= 1.0f)
                {
                    throwableComp.ArcT = 1.0f;

                    const float speedSq = glm::length2(throwableComp.VelocityWS);
                    const float minHopSpeedSq = 0.5f * 0.5f;

                    if (throwableComp.GroundBounceCount < throwableComp.MaxGroundBounces &&
                        speedSq > minHopSpeedSq)
                    {
                        // start very small extra hop
                        throwableComp.GroundBounceCount++;

                        throwableComp.ArcT = 0.0f;
                        throwableComp.ArcDuration *= 0.4f;
                        throwableComp.ArcHeightWorld *= 0.3f;

                        // Change spin on ground bounce
                        float sign = (RandomRange(0.0f, 1.0f) < 0.5f) ? -1.0f : 1.0f;
                        throwableComp.AngularSpeedZ = sign * RandomRange(throwableComp.MinSpinSpeed, throwableComp.MaxSpinSpeed);
                    }
                    else
                    {
                        throwableComp.ArcDuration = 0.0f;
                        throwableComp.ArcHeightWorld = 0.0f;
                    }
                }
            }
            // Air drag on horizontal velocity
            if (throwableComp.AirDrag > 0.0f)
            {
                float speedSq = glm::length2(throwableComp.VelocityWS);
                if (speedSq > 0.0f)
                {
                    float dragFactor = glm::max(0.0f, 1.0f - throwableComp.AirDrag * dt);
                    throwableComp.VelocityWS *= dragFactor;
                }
            }

            // Resting check: if basically stopped and arc is done, freeze it ---
            {
                const float stopSpeedSq = throwableComp.MinSpeedToStop * throwableComp.MinSpeedToStop;
                float speedSqNow = glm::length2(throwableComp.VelocityWS);

                // ArcDuration <= 0 -> no more hops; we want a fully resting state
                if (throwableComp.ArcDuration <= 0.0f && speedSqNow < stopSpeedSq)
                {
                    throwableComp.VelocityWS = glm::vec2(0.0f);
                    throwableComp.AngularSpeedZ = 0.0f;

                    // Base world position stays at last ground position
                    glm::vec2 basePos = throwableComp.GroundPosWS;

                    // ArcT is irrelevant now but we keep the same logic for safety
                    float t = glm::clamp(throwableComp.ArcT, 0.0f, 1.0f);
                    float s = std::sin(glm::pi<float>() * t);
                    float arcShape = s * s;

                    float height = glm::clamp(throwableComp.ArcHeightWorld, 0.3f, 1.5f);
                    float hopScale = (throwableComp.GroundBounceCount > 0) ? 0.2f : 1.0f;
                    float totalOffsetAmount = throwableComp.InitialLift + arcShape * height * hopScale;

                    glm::vec2 arcOffset = isoUpDirWS * totalOffsetAmount;
                    glm::vec2 renderPos = basePos + arcOffset;

                    throwableTransformComp.Translation.x = renderPos.x;
                    throwableTransformComp.Translation.y = renderPos.y;
                    throwableTransformComp.Translation.z = 0.0f;

                    // Keep whatever final rotation we ended up with
                    throwableTransformComp.Rotation.z = throwableComp.RotationZ;

                    // We’re resting, so skip bounce & spin integration
                    return;
                }
            }

            glm::vec2 oldPos = throwableComp.GroundPosWS;
            glm::vec2 newPos = oldPos + throwableComp.VelocityWS * dt;

            BounceAgainstWorld(scene, oldPos, newPos, throwableComp);
            throwableComp.GroundPosWS = newPos;

            glm::vec2 basePos = throwableComp.GroundPosWS;

            float t = glm::clamp(throwableComp.ArcT, 0.0f, 1.0f);

            float s = std::sin(glm::pi<float>() * t);
            float arcShape = s * s;

            float height = throwableComp.ArcHeightWorld;

            height = glm::clamp(height, 0.3f, 1.5f);

            float hopScale = (throwableComp.GroundBounceCount > 0) ? 0.2f : 1.0f;

            // Final vertical amount along isoUpDirWS
            float totalOffsetAmount = throwableComp.InitialLift + arcShape * height * hopScale;

            glm::vec2 arcOffset = isoUpDirWS * totalOffsetAmount;

            glm::vec2 renderPos = basePos + arcOffset;

            throwableTransformComp.Translation.x = renderPos.x;
            throwableTransformComp.Translation.y = renderPos.y;
            throwableTransformComp.Translation.z = 0.0f;

            {
                const float stopSpeedSq = throwableComp.MinSpeedToStop * throwableComp.MinSpeedToStop;
                float speedSqNow = glm::length2(throwableComp.VelocityWS);

                if (speedSqNow >= stopSpeedSq)
                {
                    throwableComp.RotationZ += throwableComp.AngularSpeedZ * dt;

                    if (throwableComp.RotationZ > glm::two_pi<float>())
                        throwableComp.RotationZ -= glm::two_pi<float>();
                    else if (throwableComp.RotationZ < -glm::two_pi<float>())
                        throwableComp.RotationZ += glm::two_pi<float>();
                }

                throwableTransformComp.Rotation.z = throwableComp.RotationZ;
            }

            {
                const float stopSpeedSq = throwableComp.MinSpeedToStop * throwableComp.MinSpeedToStop;
                if (glm::length2(throwableComp.VelocityWS) < stopSpeedSq)
                {
                    throwableComp.VelocityWS = glm::vec2(0.0f);
                    throwableComp.AngularSpeedZ = 0.0f;
                }
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
    // glm::reflect(I, N) = I - 2 * dot(N, I) * N
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

