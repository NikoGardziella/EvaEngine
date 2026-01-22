#include "pch.h"
#include "SkeletonRegistry.h"
#include <Engine/Debug/Instrumentor.h>


namespace Engine {


    int SkeletonRegistry::FindBoneIndexLower(const SkeletonAsset& skel, const char* nameLower)
    {
        for (int i = 0; i < (int)skel.boneNames.size(); ++i)
        {
            std::string n = skel.boneNames[i];
            for (char& c : n) c = (char)tolower(c);
            if (n == nameLower) return i;
        }
        return -1;
    }

    int SkeletonRegistry::FindBoneIndexContainsLower(const SkeletonAsset& skel, const char* keyLower)
    {
        for (int i = 0; i < (int)skel.boneNames.size(); ++i)
        {
            std::string n = skel.boneNames[i];
            for (char& c : n) c = (char)tolower(c);
            if (n.find(keyLower) != std::string::npos) return i;
        }
        return -1;
    }

    void SkeletonRegistry::BuildUpperBodyMask_Player(const SkeletonAsset& skel, std::vector<float>& outMask)
    {
        EE_PROFILE_FUNCTION();
        // names are from mixamo


        // Split at spine
        int spine = FindBoneIndexLower(skel, "spine");
        if (spine < 0) spine = FindBoneIndexContainsLower(skel, "spine");

        BuildDescendantMask(skel, spine, outMask);

        // Ensure pelvis is 0 (even if skeleton hierarchy is weird)
        int pelvis = FindBoneIndexLower(skel, "pelvis");
        if (pelvis < 0) pelvis = FindBoneIndexContainsLower(skel, "pelvis");
        if (pelvis >= 0 && pelvis < (int)outMask.size())
            outMask[pelvis] = 0.0f;

        // Soften the seam
        if (spine >= 0 && spine < (int)outMask.size())
            outMask[spine] = 0.4f;

        int chest = FindBoneIndexLower(skel, "chest");
        if (chest >= 0 && chest < (int)outMask.size())
            outMask[chest] = 1.0f;
    }

    void SkeletonRegistry::BuildDescendantMask(const SkeletonAsset& skel, int rootIdx, std::vector<float>& outMask)
    {
        const int boneCount = (int)skel.boneNames.size();
        outMask.assign(boneCount, 0.0f);
        if (rootIdx < 0 || rootIdx >= boneCount)
            return;

        // Build children lists for fast traversal
        std::vector<std::vector<int>> children;
        children.resize(boneCount);
        for (int i = 0; i < boneCount; ++i)
        {
            int p = skel.parent[i];
            if (p >= 0 && p < boneCount)
                children[p].push_back(i);
        }

        // DFS/BFS from rootIdx to mark all descendants
        std::vector<int> stack;
        stack.push_back(rootIdx);

        while (!stack.empty())
        {
            int b = stack.back();
            stack.pop_back();

            outMask[b] = 1.0f;

            for (int c : children[b])
                stack.push_back(c);
        }
    }



}