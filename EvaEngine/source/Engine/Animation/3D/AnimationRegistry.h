#pragma once
#include "glm/glm.hpp"
#include "SkeletonRegistry.h"
#include <glm/gtc/quaternion.hpp>

namespace Engine
{
    struct AnimChannel
    {
        uint16_t bone; // bone index
        std::vector<float> times;    // strictly increasing
        std::vector<glm::quat> R;
        std::vector<glm::vec3> T;
        std::vector<glm::vec3> S;
    };

    struct AnimationClip
    {
        uint32_t id;
        uint32_t skeletonId;
        float duration = 0.0f;
        std::vector<AnimChannel> channels;
        std::string name;
        // optional compression metadata / cached indices
    };

    class AnimationRegistry
    {

    public:

        AnimationRegistry(const Ref<SkeletonRegistry> skelReg = nullptr)
            : m_skelReg(skelReg) 
        {
        }


        uint32_t LoadGLTFClip(const char* path, uint32_t skeletonId, const char* clipName = nullptr);
        const AnimationClip& AnimationRegistry::Get(uint32_t id) const
        {
            static AnimationClip dummy{}; 

            if (id >= m_clips.size()) 
            {
                EE_CORE_ERROR("[AnimationRegistry] Get: invalid id {} (clip count = {})",
                    id, (uint32_t)m_clips.size());
                return dummy;
            }
            return m_clips[id];
        }

        void SetSkeletonRegistry(const Ref<SkeletonRegistry>& skelReg)
        {
            m_skelReg = skelReg;
        }

        const SkeletonRegistry& GetSkeletonRegistry() const { return *m_skelReg;  }


        AnimClipId RegisterAnimation(const std::string& key, SkeletonId skeleton, AnimationClip& clip);

        AnimClipId FindAnimClipId(const std::string& key) const;

        const AnimationClip& GetAnimationClip(AnimClipId id) const;

        const AnimationClip* FindAnimationClip(const std::string& key) const;

        const std::string& GetKey(AnimClipId id) const;

    private:

        Ref<SkeletonRegistry> m_skelReg = nullptr;
        std::vector<AnimationClip> m_clips;
        std::vector<std::string>   m_clipToKey;
        std::unordered_map<std::string, AnimClipId> m_keyToId;
    };

}

