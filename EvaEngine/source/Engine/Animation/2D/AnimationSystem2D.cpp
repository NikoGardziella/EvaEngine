#include "pch.h"

#include "AnimationSystem2D.h"
#include "Engine/Animation/2D/AnimationBank2D.h"
#include "Engine/Animation/2D/SpriteInstanceData.h"
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Components/Physics/PhysicsComponent.h>
#include <Engine/Renderer/Renderer2D/VulkanRenderer2D.h>
#include <Engine/Core/Log.h>
#include "AnimationToRenderer.h"
#include "DirectionSelector.h"
#include "Engine/Scene/Entity.h"

namespace Engine {


    static float WrapAngle(float a)
    {
        const float twoPi = glm::two_pi<float>();
        a = fmod(a, twoPi);
        if (a < 0.0f) a += twoPi;
        return a;
    }

    static float ShortestAngleDelta(float from, float to)
    {
        float diff = WrapAngle(to) - WrapAngle(from);
        if (diff > glm::pi<float>())  diff -= glm::two_pi<float>();
        if (diff < -glm::pi<float>()) diff += glm::two_pi<float>();
        return diff;
    }

    // rotate 'current' toward 'target' by at most 'maxStep' radians
    static float RotateTowards(float current, float target, float maxStep)
    {
        float delta = ShortestAngleDelta(current, target);
        if (glm::abs(delta) <= maxStep)
            return WrapAngle(target);

        return WrapAngle(current + glm::clamp(delta, -maxStep, maxStep));
    }

    static float Dir8CenterAngle(Dir8 d)
    {
        const float step = glm::two_pi<float>() / 8.0f; // 45°
        switch (d)
        {
        case Dir8::E:  return 0.0f * step;  // 0°  (pointing +X)
        case Dir8::NE: return 1.0f * step;  // 45°
        case Dir8::N:  return 2.0f * step;  // 90° (+Y)
        case Dir8::NW: return 3.0f * step;  // 135°
        case Dir8::W:  return 4.0f * step;  // 180° (-X)
        case Dir8::SW: return 5.0f * step;  // 225°
        case Dir8::S:  return 6.0f * step;  // 270° (-Y)
        case Dir8::SE: return 7.0f * step;  // 315°
        default:       return 0.0f;
        }
    }

 

    void AnimationSystem2D::Update(float dt, Scene* scene)
    {
        scene->ForEach<Animation2DComponent, AnimatorStateComponent, TransformComponent>(
            [this, dt](Entity e, Animation2DComponent& ac, AnimatorStateComponent& as, TransformComponent& transformComp)
            {
                const Animation2DClipRuntime* clip = m_bank->Get2DClip(ac.clipId);
                if (!clip || clip->grid.cols == 0 || clip->grid.rows == 0)
                    return;

                glm::vec2 vel = glm::vec2(0.0f); // TODO: real velocity when you want

                // 1) target angle from aim
                float targetAngle = ac.aimRadians;

                // 2) smooth
                const float turnSpeed = (ac.turnSpeed > 0.0f)
                    ? ac.turnSpeed
                    : glm::radians(720.0f);
                float maxStep = turnSpeed * dt;
                ac.facingRadians = RotateTowards(ac.facingRadians, targetAngle, maxStep);

                // 3) pick dir from SMOOTHED angle (same logic as before)
                Dir8 dir = AnimationSystem2D::SelectDirection(ac, vel, ac.facingRadians);
                ac.direction = dir;

                const uint32_t dirIdx = static_cast<uint32_t>(dir);
                const uint32_t mappedRow = (dirIdx < 8)
                    ? uint32_t(clip->dirToRow[dirIdx])
                    : 0u;
                const uint32_t row = std::min(mappedRow, uint32_t(clip->grid.rows - 1));

                // 4) advance anim
                Advance(as.state, clip->grid, dt);
                const uint32_t col = std::min<uint32_t>(as.state.frame, clip->grid.cols - 1);

                const uint32_t idx = row * clip->grid.cols + col;
                const auto& f = clip->uvTable[idx];
                const glm::uvec2 uvMin16 = glm::uvec2(f.uvMin16);
                const glm::uvec2 uvMax16 = glm::uvec2(f.uvMax16);

                const glm::vec2 sizeWorld =
                    glm::vec2(clip->frameSizePx) / clip->pixelsPerUnit;

                const glm::vec2 center = glm::vec2(transformComp.Translation);
                const float zKey = center.y * 1024.0f;

                glm::vec2 offset = glm::vec2{ 0.0f, -1.0f };

                // 5) compute delta angle between continuous facing and row's baked angle
                float rowAngle = Dir8CenterAngle(dir);
                float rotationForSprite = ac.facingRadians - rowAngle;

                // optional: wrap to [-pi, pi] if you like
                // rotationForSprite = WrapAngle(rotationForSprite);

                transformComp.Rotation.z = rotationForSprite;

                VulkanRenderer2D::SubmitAnimationSpriteInstance(
                    center + offset,
                    zKey,
                    clip->grid.textureIndex,
                    uvMin16,
                    uvMax16,
                    sizeWorld,
                    rotationForSprite
                );
            });
    }




    Dir8 AnimationSystem2D::SelectDirection(const Animation2DComponent& ac, const glm::vec2& vel, float aim)
    {
        static thread_local DirectionSelector selector;
        switch (ac.dirMode) {
        case 0: // velocity
            return selector.FromVector(vel);
        case 1: // aim
            return selector.FromAngle(aim);
        case 2: // manual
        default:
            return ac.direction;
        }
    }

    void AnimationSystem2D::Advance(AnimatorState& st, const AnimationClipGrid& grid, float dt)
    {
        const float fps = std::max(0.001f, grid.fps) * std::max(0.001f, st.speed);
        const float frameDur = 1.0f / fps;
        st.time += dt;

        while (st.time >= frameDur)
        {
            st.time -= frameDur;
            if (++st.frame >= grid.cols) st.frame = grid.loop ? 0 : (grid.cols - 1);
        }
    }

} 
