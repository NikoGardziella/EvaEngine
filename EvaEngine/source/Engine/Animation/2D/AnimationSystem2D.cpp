#include "pch.h"

#include "AnimationSystem2D.h"
#include "Engine/Animation/2D/AnimationBank2D.h"
#include "Engine/Animation/2D/SpriteInstanceData.h"
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Components/Physics/PhysicsComponent.h>
#include <Engine/Renderer/VulkanRenderer2D.h>
#include <Engine/Core/Log.h>
#include "AnimationToRenderer.h"
#include "DirectionSelector.h"

namespace Engine {

    void AnimationSystem2D::Update(float dt, Scene* scene)
    {


        scene->ForEach<AnimationComponent, AnimatorStateComponent, TransformComponent>(
            [this, dt](Entity e, AnimationComponent& ac, AnimatorStateComponent& as, TransformComponent& transformComp)
            {
                const Animation2DClipRuntime* clip = m_bank->Get2DClip(ac.clipId);
                if (!clip || clip->grid.cols == 0 || clip->grid.rows == 0) return;

                glm::vec2 vel = glm::vec2(0.0f); // TODO: read from PhysicsComponent
                Dir8 dir = AnimationSystem2D::SelectDirection(ac, vel, ac.aimRadians);

                const uint32_t dirIdx = static_cast<uint32_t>(dir);
                const uint32_t mappedRow = (dirIdx < 8) ? uint32_t(clip->dirToRow[dirIdx]) : 0u;
                const uint32_t row = std::min(mappedRow, uint32_t(clip->grid.rows - 1));

                Advance(as.state, clip->grid, dt);
                const uint32_t col = std::min<uint32_t>(as.state.frame, clip->grid.cols - 1);

                const uint32_t idx = row * clip->grid.cols + col;
                const auto& f = clip->uvTable[idx];
                const glm::uvec2 uvMin16 = glm::uvec2(f.uvMin16);
                const glm::uvec2 uvMax16 = glm::uvec2(f.uvMax16);

                const glm::vec2 sizeWorld = glm::vec2(clip->frameSizePx) / clip->pixelsPerUnit;

                const glm::vec2 center = glm::vec2(transformComp.Translation);
                const float zKey = center.y * 1024.0f;


                glm::vec2 offset = glm::vec2{ 0.0f, -1.0f };

                VulkanRenderer2D::SubmitAnimationSpriteInstance(center + offset, zKey,
                    clip->grid.textureIndex, uvMin16, uvMax16, sizeWorld);
            });
    }

    Dir8 AnimationSystem2D::SelectDirection(const AnimationComponent& ac, const glm::vec2& vel, float aim)
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
