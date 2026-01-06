#pragma once
#include "glm/glm.hpp"

namespace Engine {

    struct SkeletonAsset
    {
        uint32_t id;
        std::vector<std::string> boneNames;
        std::vector<int16_t>    parent;         // parent index per bone (-1 for root)
        std::vector<glm::mat4>  invBind;      // inverse bind matrices
        std::vector<glm::mat4>  restLocal; // local TRS matrix from glTF node
        std::vector<int>        jointNodes;
        // glTF node index per bone     
    };

    class SkeletonRegistry {

    public:

        


    public:


        static int FindBoneIndexLower(const SkeletonAsset& skel, const char* nameLower);

        static int FindBoneIndexContainsLower(const SkeletonAsset& skel, const char* keyLower);

        static void BuildUpperBodyMask_Player(const SkeletonAsset& skel, std::vector<float>& outMask);




        const SkeletonAsset& SkeletonRegistry::Get(uint32_t id) const
        {
            return m_skeletonAssets[id];
        }

        uint32_t Register(const SkeletonAsset& s)
        {
            SkeletonAsset copy = s;
            copy.id = (uint32_t)m_skeletonAssets.size();
            m_skeletonAssets.push_back(copy);
            /*
            for (size_t i = 0; i < copy.boneNames.size(); i++)
            {
                EE_CORE_INFO("bone name; {}", copy.boneNames[i].c_str());
                
            }
            */

            return copy.id;
        }

        static uint32_t FindBoneContains(const SkeletonAsset& s, std::string_view token)
        {
            
            for (uint32_t i = 0; i < (uint32_t)s.boneNames.size(); ++i)
                if (s.boneNames[i].find(token) != std::string::npos)
                    return i;
            return -1;
        }



    private:
        static void BuildDescendantMask(const SkeletonAsset& skel, int rootIdx, std::vector<float>& outMask);

    private:

        std::vector<SkeletonAsset> m_skeletonAssets;
    };
}


