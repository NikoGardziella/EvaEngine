#include "pch.h"
#include "AnimationSystem3D.h"

#include <Engine/Scene/Components/Render/3D/SkeletonComponent.h>
#include <Engine/Scene/Components/Render/3D/AnimatorComponent.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Animation/3D/Utils/AnimationUtils.h>
#include <Engine/Scene/Components/Animation/NpcAnimationControllerComponent.h>

namespace Engine {




    void AnimationSystem3D::Update(Scene* scene, float dt, const SkeletonRegistry& skelReg, const AnimationRegistry& animReg,
        BonePaletteBuffer& palette)
    {
        EE_PROFILE_FUNCTION();

        AnimScratch3D scratch;

        scene->ForEach<SkeletonComponent, Animator3DComponent>([&]
        (Entity entity, SkeletonComponent& skel, Animator3DComponent& an)
            {
                if (skel.boneCount == 0 || skel.skeletonId == 0xFFFFFFFFu)
                    return;

                // 1) Advance times
                AdvanceTime(an.clipA, an.timeA, dt, an.playbackSpeed, animReg, an.loopAclip);

                // For base crossfade, clipB is a looping base; otherwise overlay is non-looping
                //const bool loopB = (ctrl.baseXFadeActive != 0);
                AdvanceTime(an.clipB, an.timeB, dt, an.playbackSpeed, animReg, false);


                const SkeletonAsset& sasset = skelReg.Get(skel.skeletonId);
                const uint32_t boneCount = skel.boneCount;

                // 2) Scratch buffers
                AnimationUtils::EnsureSize(scratch.TA, boneCount);
                AnimationUtils::EnsureSize(scratch.RA, boneCount);
                AnimationUtils::EnsureSize(scratch.SA, boneCount);

                AnimationUtils::EnsureSize(scratch.TB, boneCount);
                AnimationUtils::EnsureSize(scratch.RB, boneCount);
                AnimationUtils::EnsureSize(scratch.SB, boneCount);

                AnimationUtils::EnsureSize(scratch.T, boneCount);
                AnimationUtils::EnsureSize(scratch.R, boneCount);
                AnimationUtils::EnsureSize(scratch.S, boneCount);

                auto& locT = scratch.T;
                auto& locR = scratch.R;
                auto& locS = scratch.S;
                auto& model = scratch.model;
                auto& finalMats = scratch.finalMats;

                // 3) Identity pose (or bind pose if you add it later)
                for (uint32_t i = 0; i < boneCount; ++i)
                {
                    glm::vec3 restT; glm::quat restR; glm::vec3 restS;
                    AnimationUtils::DecomposeTRS(sasset.restLocal[i], restT, restR, restS);

                    scratch.TA[i] = restT;
                    scratch.RA[i] = restR;
                    scratch.SA[i] = restS;

                    scratch.TB[i] = restT;
                    scratch.RB[i] = restR;
                    scratch.SB[i] = restS;
                }
                //EE_CORE_INFO("blend={}, clipA={}, clipB={}", an.blend, (uint32_t)an.clipA, (uint32_t)an.clipB);
                float wB = glm::clamp(an.blend, 0.0f, 1.0f);

                // If clipB is missing, it must not contribute
                if (an.clipA == INVALID_CLIP && an.clipB != INVALID_CLIP) wB = 1.0f;
                if (an.clipB == INVALID_CLIP) wB = 0.0f;

                float wA = 1.0f - wB;

                ApplyClipFullPose(an.clipA, an.timeA, boneCount, animReg, scratch.TA, scratch.RA, scratch.SA);
                ApplyClipFullPose(an.clipB, an.timeB, boneCount, animReg, scratch.TB, scratch.RB, scratch.SB);
               




                //EE_CORE_INFO("Entity {}: clipA={}, clipB={}", (uint32_t)entity, an.clipA, an.clipB);

                const auto& parent = sasset.parent;
                const auto& invBind = sasset.invBind;

                for (uint32_t i = 0; i < boneCount; ++i)
                {
                    scratch.T[i] = glm::mix(scratch.TA[i], scratch.TB[i], wB);
                    scratch.S[i] = glm::mix(scratch.SA[i], scratch.SB[i], wB);
                    glm::quat a = scratch.RA[i];
                    glm::quat b = scratch.RB[i];
                    if (glm::dot(a, b) < 0.0f) b = -b;
                    scratch.R[i] = glm::normalize(glm::slerp(a, b, wB));
                }
                

                model.resize(boneCount);
                finalMats.resize(boneCount);




                for (uint32_t i = 0; i < boneCount; ++i)
                {
                    const glm::mat4 local = AnimationUtils::TRS(locT[i], locR[i], locS[i]);

                    const int p = parent[i];
                    const glm::mat4 world = (p >= 0) ? (model[p] * local) : local;
                    model[i] = world;
                    an.boneModel[i] = world;

                    const glm::mat4 invB = (i < invBind.size()) ? invBind[i] : glm::mat4(1.0f);
                    finalMats[i] = world * invB;
                }

                // 3) Allocate slice and upload
                uint32_t base = VulkanRenderer3D::GetBoneCursor();
                skel.boneBase = base;

                for (uint32_t i = 0; i < boneCount; ++i)
                {

                    VulkanRenderer3D::SubmitBone(finalMats[i]);
                }
            });
    }

    void AnimationSystem3D::ApplyClipFullPose(uint32_t clipId, float t, uint32_t boneCount, const AnimationRegistry& animReg,
        std::vector<glm::vec3>& T, std::vector<glm::quat>& R, std::vector<glm::vec3>& S)
    {
        if (clipId == 0xFFFFFFFFu)
            return;

        const AnimationClip& clip = animReg.Get(clipId);

        for (const AnimChannel& ch : clip.channels)
        {
            uint32_t b = ch.bone;
            if (b >= boneCount) continue;

            glm::vec3 tS;
            glm::quat rS;
            glm::vec3 sS;
            SampleChannel(ch, t, tS, rS, sS);

            T[b] = tS;
            R[b] = rS;
            S[b] = sS;
        }
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


    

    float AnimationSystem3D::AdvanceTime(uint32_t clipId, float& t, float dt, float playbackSpeed,
        const AnimationRegistry& animReg, bool loop)
    {
        if (clipId == 0xFFFFFFFFu) return 0.0f;

        const AnimationClip& clip = animReg.Get(clipId);
        t += dt * playbackSpeed;

        if (clip.duration > 0.0f)
        {
            if (loop)
            {
                while (t >= clip.duration) t -= clip.duration;
                while (t < 0.0f)          t += clip.duration;
            }
            else
            {
                if (t < 0.0f) t = 0.0f;
                if (t > clip.duration) t = clip.duration; // clamp and hold last frame
            }
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