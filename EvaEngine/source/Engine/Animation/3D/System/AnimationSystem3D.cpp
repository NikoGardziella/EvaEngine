#include "pch.h"
#include "AnimationSystem3D.h"
#include <Engine/Scene/Components/Render/3D/SkeletonComponent.h>
#include <Engine/Scene/Components/Render/3D/AnimatorComponent.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Animation/3D/Utils/AnimationUtils.h>


namespace Engine {

    static inline int FindKey(const std::vector<float>& times, float t)
    {
        // return index i where times[i] <= t < times[i+1]; clamp to last-1
        if (times.empty()) return 0;
        if (t <= times.front()) return 0;
        if (t >= times.back()) return (int)times.size() - 2;
        // binary search
        int lo = 0, hi = (int)times.size() - 1;
        while (hi - lo > 1)
        {
            int mid = (lo + hi) >> 1;
            if (t >= times[mid]) lo = mid; else hi = mid;
        }
        return lo;
    }

    static inline void SampleChannel(const AnimChannel& ch, float t, glm::vec3& T, glm::quat& R, glm::vec3& S)
    {
        const auto& tt = ch.times;
        int i = FindKey(tt, t);
        float t0 = tt[i];
        float t1 = tt[i + 1];
        float a = (t1 > t0) ? (t - t0) / (t1 - t0) : 0.0f;

        // Guard sizes: channels may omit some tracks; assume arrays sized like times or size 1
        auto lerpV3 = [&](const std::vector<glm::vec3>& v, glm::vec3 def)->glm::vec3 {
            if (v.empty()) return def;
            if (i + 1 >= (int)v.size()) return v.back();
            return glm::mix(v[i], v[i + 1], a);
            };
        auto slerpQ = [&](const std::vector<glm::quat>& v, glm::quat def)->glm::quat {
            if (v.empty()) return def;
            if (i + 1 >= (int)v.size()) return glm::normalize(v.back());
            return glm::normalize(glm::slerp(v[i], v[i + 1], a));
            };

        T = lerpV3(ch.T, glm::vec3(0));
        S = lerpV3(ch.S, glm::vec3(1));
        R = slerpQ(ch.R, glm::quat(1, 0, 0, 0));
    }


    void AnimationSystem3D::Update(Scene* scene, float dt, const SkeletonRegistry& skelReg,
        const AnimationRegistry& animReg, BonePaletteBuffer& palette)
    {
        EE_PROFILE_FUNCTION();

        scene->ForEach<SkeletonComponent, AnimatorComponent>([&](Entity e, SkeletonComponent& skel, AnimatorComponent& an) {
            if (skel.boneCount == 0) return;

            // Advance times
            auto adv = [&](uint32_t clipId, float& t)->float {
                if (clipId == 0xFFFFFFFFu) return 0.0f;
                const auto& clip = animReg.Get(clipId);
                t += dt * an.playbackSpeed;
                // loop
                while (t >= clip.duration && clip.duration > 0.0f) t -= clip.duration;
                if (t < 0.0f && clip.duration > 0.0f) t = fmodf(t, clip.duration);
                return clip.duration;
                };
            adv(an.clipA, an.timeA);
            adv(an.clipB, an.timeB);

            // Build local pose arrays
            std::vector<glm::vec3> locT(skel.boneCount, glm::vec3(0.0f));
            std::vector<glm::quat> locR(skel.boneCount, glm::quat(1, 0, 0, 0));
            std::vector<glm::vec3> locS(skel.boneCount, glm::vec3(1.0f));

            auto applyClip = [&](uint32_t clipId, float t, float weight) {
                if (clipId == 0xFFFFFFFFu || weight <= 0.0f) return;
                const auto& clip = animReg.Get(clipId);
                for (const auto& ch : clip.channels) {
                    glm::vec3 T; glm::quat R; glm::vec3 S;
                    SampleChannel(ch, t, T, R, S);
                    uint32_t b = ch.bone;
                    // additively blend in local space
                    // For simplicity: lerp T,S; nlerp R over weight, assuming base is default pose
                    locT[b] = glm::mix(locT[b], T, weight);
                    locS[b] = glm::mix(locS[b], S, weight);
                    locR[b] = glm::normalize(glm::slerp(locR[b], R, weight));
                }
                };

            float wB = glm::clamp(an.blend, 0.0f, 1.0f);
            float wA = 1.0f - wB;
            applyClip(an.clipA, an.timeA, wA);
            applyClip(an.clipB, an.timeB, wB);

            // Build model-space bone transforms by hierarchy
            const auto& sasset = skelReg.Get(skel.skeletonId);
            std::vector<glm::mat4> model(skel.boneCount, glm::mat4(1.0f));
            for (uint32_t i = 0; i < skel.boneCount; ++i) {
                glm::mat4 M = AnimationUtils::TRS(locT[i], locR[i], locS[i]);
                int parent = (i < sasset.parent.size()) ? sasset.parent[i] : -1;
                if (parent >= 0) model[i] = model[parent] * M; else model[i] = M;
            }

            // Final matrices for skinning = model * invBind
            std::vector<glm::mat4> finalMats(skel.boneCount);
            for (uint32_t i = 0; i < skel.boneCount; ++i) {
                const glm::mat4 invB = (i < sasset.invBind.size()) ? sasset.invBind[i] : glm::mat4(1.0f);
                finalMats[i] = model[i] * invB;
            }

            // Upload to bone palette
            if (skel.boneBase == 0xFFFFFFFFu)
            {
                // allocate lazily if not allocated yet
                skel.boneBase = palette.Allocate(skel.boneCount);
            }
            palette.Upload(skel.boneBase, finalMats.data(), skel.boneCount);

            
            });
    }

} 
