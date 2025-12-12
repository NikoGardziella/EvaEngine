#include "pch.h"
#include "AnimationRegistry.h"


namespace Engine {

    uint32_t AnimationRegistry::LoadGLTFClip(const char* path, uint32_t skeletonId, const char* clipName)
    {
        return 0;
    }

    AnimClipId AnimationRegistry::RegisterAnimation(const std::string& key, SkeletonId skeleton, AnimationClip& clip)
    {
        auto it = m_keyToId.find(key);
        if (it != m_keyToId.end())
        {
            AnimClipId id = it->second;
            EE_CORE_ASSERT(m_clips[id].skeletonId == skeleton,
                "Trying to re-register clip with different skeleton");
            return id;
        }

        AnimClipId id = (AnimClipId)m_clips.size();
        clip.id = id;
        m_clips.push_back(clip);
        m_keyToId[key] = id;
        m_clipToKey.push_back(key);

        EE_CORE_INFO("registered animation {}, with skeletonId {}", key, skeleton);
        return id;
    }

    AnimClipId AnimationRegistry::FindAnimClipId(const std::string& key) const
    {
        auto it = m_keyToId.find(key);
        return (it != m_keyToId.end()) ? it->second : INVALID_CLIP;
    }

    const AnimationClip& AnimationRegistry::GetAnimationClip(AnimClipId id) const
    {
        EE_CORE_ASSERT(id < m_clips.size(), "Invalid AnimClipId");
        return m_clips[id];
    }

    const AnimationClip* AnimationRegistry::FindAnimationClip(const std::string& key) const
    {
        auto it = m_keyToId.find(key);
        if (it == m_keyToId.end())
            return nullptr;

        return &m_clips[it->second];
    }

    const std::string& AnimationRegistry::GetKey(AnimClipId id) const
    {
        EE_CORE_ASSERT(id < m_clipToKey.size(), "Invalid AnimClipId");
        return m_clipToKey[id];
    }

}