#pragma once
#include "glm/glm.hpp"

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

    class AnimationRegistry {

    private:
       

        struct AnimationClip 
        {
            uint32_t id;
            float duration = 0.0f;
            std::vector<AnimChannel> channels;
            // optional compression metadata / cached indices
        };

    public:
        uint32_t LoadGLTFClip(const char* path, uint32_t skeletonId, const char* clipName = nullptr);
        const AnimationClip& Get(uint32_t id) const;
    };

}

