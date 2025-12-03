#pragma once
#include "glm/glm.hpp"

namespace Engine {

    struct SkeletonAsset
    {
        uint32_t id;
        std::vector<int16_t>    parent;         // parent index per bone (-1 for root)
        std::vector<glm::mat4>  invBind;      // inverse bind matrices
        std::vector<glm::mat4>  restLocal; // local TRS matrix from glTF node
        std::vector<int>        jointNodes; // glTF node index per bone     
    };

    class SkeletonRegistry {

    public:

        


    public:

        const SkeletonAsset& SkeletonRegistry::Get(uint32_t id) const
        {
            return m_skeletonAssets[id];
        }

        uint32_t Register(const SkeletonAsset& s)
        {
            SkeletonAsset copy = s;
            copy.id = (uint32_t)m_skeletonAssets.size();
            m_skeletonAssets.push_back(copy);
            return copy.id;
        }

    private:

        std::vector<SkeletonAsset> m_skeletonAssets;
    };
}


