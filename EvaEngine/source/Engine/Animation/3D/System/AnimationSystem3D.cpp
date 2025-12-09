#include "pch.h"
#include "AnimationSystem3D.h"

#include <Engine/Scene/Components/Render/3D/SkeletonComponent.h>
#include <Engine/Scene/Components/Render/3D/AnimatorComponent.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Animation/3D/Utils/AnimationUtils.h>

namespace Engine {

    void AnimationSystem3D::Update(Scene* scene, float dt, const SkeletonRegistry& skelReg, const AnimationRegistry& animReg,
        BonePaletteBuffer& palette)
    {
        EE_PROFILE_FUNCTION();

        AnimScratch3D scratch;

        scene->ForEach<SkeletonComponent, AnimatorComponent>([&](Entity entity, SkeletonComponent& skel, AnimatorComponent& an)
            {
                if (skel.boneCount == 0 || skel.skeletonId == 0xFFFFFFFFu)
                    return;

                // 1) Advance times
                AdvanceTime(an.clipA, an.timeA, dt, an.playbackSpeed, animReg);
                AdvanceTime(an.clipB, an.timeB, dt, an.playbackSpeed, animReg);

                const SkeletonAsset& sasset = skelReg.Get(skel.skeletonId);
                const uint32_t boneCount = skel.boneCount;

                // 2) Scratch buffers
                AnimationUtils::EnsureSize(scratch.T, boneCount);
                AnimationUtils::EnsureSize(scratch.R, boneCount);
                AnimationUtils::EnsureSize(scratch.S, boneCount);
                AnimationUtils::EnsureSize(scratch.model, boneCount);
                AnimationUtils::EnsureSize(scratch.finalMats, boneCount);

                auto& locT = scratch.T;
                auto& locR = scratch.R;
                auto& locS = scratch.S;
                auto& model = scratch.model;
                auto& finalMats = scratch.finalMats;

                // 3) Identity pose (or bind pose if you add it later)
                for (uint32_t i = 0; i < boneCount; ++i)
                {
                    locT[i] = glm::vec3(0.0f);
                    locR[i] = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                    locS[i] = glm::vec3(1.0f);
                }

                const float wB = glm::clamp(an.blend, 0.0f, 1.0f);
                const float wA = 1.0f - wB;

                ApplyClip(an.clipA, an.timeA, wA, boneCount, animReg, locT, locR, locS);
                ApplyClip(an.clipB, an.timeB, wB, boneCount, animReg, locT, locR, locS);
                //EE_CORE_INFO("Entity {}: clipA={}, clipB={}", (uint32_t)entity, an.clipA, an.clipB);

                const auto& parent = sasset.parent;
                const auto& invBind = sasset.invBind;

                for (uint32_t i = 0; i < boneCount; ++i)
                {
                    const glm::mat4 local = AnimationUtils::TRS(locT[i], locR[i], locS[i]);

                    const int p = parent[i];
                    glm::mat4 world = (p >= 0) ? (model[p] * local) : local;
                    model[i] = world;

                    const glm::mat4 invB =
                        (i < invBind.size()) ? invBind[i] : glm::mat4(1.0f);

                    finalMats[i] = world * invB;
                }


                uint32_t base = VulkanRenderer3D::GetBoneCursor();
                skel.boneBase = base;   // each skeleton gets its own slice
               

                for (uint32_t i = 0; i < boneCount; ++i)
                {
                    VulkanRenderer3D::SubmitBone(finalMats[i]);
                }

                // DEBUG
                glm::vec3 t0 = glm::vec3(finalMats[0][3]);
                EE_CORE_INFO("Entity {}: clipA={}, base={}, T0=({},{},{})",
                    (uint32_t)entity, an.clipA, skel.boneBase, t0.x, t0.y, t0.z);

            });
    }


    int AnimationSystem3D::FindKey(const std::vector<float>& times, float t)
    {
        const int n = (int)times.size();
        if (n == 0) return 0;
        if (n == 1) return 0;

        if (t <= times.front()) return 0;
        if (t >= times.back())  return n - 2;

        int lo = 0;
        int hi = n - 1;
        while (hi - lo > 1)
        {
            int mid = (lo + hi) >> 1;
            if (t >= times[mid]) lo = mid;
            else                 hi = mid;
        }
        return lo;
    }

    void AnimationSystem3D::SampleChannel(const AnimChannel& ch, float t, glm::vec3& T,
        glm::quat& R,  glm::vec3& S)
    {
        const auto& tt = ch.times;
        const int n = (int)tt.size();

        if (n == 0)
        {
            T = glm::vec3(0.0f);
            S = glm::vec3(1.0f);
            R = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            return;
        }

        if (n == 1)
        {
            T = ch.T.empty() ? glm::vec3(0.0f) : ch.T[0];
            S = ch.S.empty() ? glm::vec3(1.0f) : ch.S[0];
            R = ch.R.empty()
                ? glm::quat(1.0f, 0.0f, 0.0f, 0.0f)
                : glm::normalize(ch.R[0]);
            return;
        }

        const int   i = FindKey(tt, t);
        const float t0 = tt[i];
        const float t1 = tt[i + 1];
        float a = (t1 > t0) ? (t - t0) / (t1 - t0) : 0.0f;
        a = glm::clamp(a, 0.0f, 1.0f);

        // Position
        if (ch.T.empty())
        {
            T = glm::vec3(0.0f);
        }
        else
        {
            const int ti0 = glm::clamp(i, 0, (int)ch.T.size() - 1);
            const int ti1 = glm::clamp(i + 1, 0, (int)ch.T.size() - 1);
            T = glm::mix(ch.T[ti0], ch.T[ti1], a);
        }

        // Scale
        if (ch.S.empty())
        {
            S = glm::vec3(1.0f);
        }
        else
        {
            const int si0 = glm::clamp(i, 0, (int)ch.S.size() - 1);
            const int si1 = glm::clamp(i + 1, 0, (int)ch.S.size() - 1);
            S = glm::mix(ch.S[si0], ch.S[si1], a);
        }

        // Rotation
        if (ch.R.empty())
        {
            R = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }
        else
        {
            const int ri0 = glm::clamp(i, 0, (int)ch.R.size() - 1);
            const int ri1 = glm::clamp(i + 1, 0, (int)ch.R.size() - 1);
            R = glm::normalize(glm::slerp(ch.R[ri0], ch.R[ri1], a));
        }
    }


    

    float AnimationSystem3D::AdvanceTime(uint32_t clipId,  float& t, float dt, float playbackSpeed,
        const AnimationRegistry& animReg)
    {
        if (clipId == 0xFFFFFFFFu)
            return 0.0f;

        const AnimationClip& clip = animReg.Get(clipId);
        t += dt * playbackSpeed;

        if (clip.duration > 0.0f)
        {
            while (t >= clip.duration) t -= clip.duration;
            while (t < 0.0f)          t += clip.duration;
        }

        return clip.duration;
    }

    void AnimationSystem3D::ApplyClip(uint32_t clipId, float t, float weight, uint32_t boneCount, const AnimationRegistry& animReg,
        std::vector<glm::vec3>& locT, std::vector<glm::quat>& locR, std::vector<glm::vec3>& locS)
    {
        if (clipId == 0xFFFFFFFFu || weight <= 0.0f)
            return;

        const AnimationClip& clip = animReg.Get(clipId);
        const size_t channelCount = clip.channels.size();

        for (size_t ci = 0; ci < channelCount; ++ci)
        {
            const AnimChannel& ch = clip.channels[ci];

            glm::vec3 T;
            glm::quat R;
            glm::vec3 S;
            SampleChannel(ch, t, T, R, S);

            const uint32_t b = ch.bone;
            if (b >= boneCount)
                continue;

            locT[b] = glm::mix(locT[b], T, weight);
            locS[b] = glm::mix(locS[b], S, weight);
            locR[b] = glm::normalize(glm::slerp(locR[b], R, weight));
        }
    }


} 