#include "pch.h"
#include "SceneUtils.h"

namespace Engine {



    bool SceneUtils::StrContains(const std::string& s, const char* needle)
    {
        return s.find(needle) != std::string::npos;
    }

    EnemyPieceType SceneUtils::ClassifyPieceTypeFromSubmeshName(const std::string& smName)
    {
        // Your submesh names look like: "Z_L_Forearm_prim0"
        if (StrContains(smName, "Head")) return EnemyPieceType::Head;

        if (StrContains(smName, "BodyTop")) return EnemyPieceType::Torso;
        if (StrContains(smName, "Body"))    return EnemyPieceType::Torso;

        if (StrContains(smName, "Hip")) return EnemyPieceType::Hip;

        if (StrContains(smName, "Z_L_Upperarm")) return EnemyPieceType::ArmL_Upper;
        if (StrContains(smName, "Z_L_Forearm"))  return EnemyPieceType::ArmL_Forearm;
        if (StrContains(smName, "Z_L_ArmPalm"))  return EnemyPieceType::ArmL_Palm;

        if (StrContains(smName, "Z_R_Upperarm")) return EnemyPieceType::ArmR_Upper;
        if (StrContains(smName, "Z_R_Forearm"))  return EnemyPieceType::ArmR_Forearm;
        if (StrContains(smName, "Z_R_ArmPalm"))  return EnemyPieceType::ArmR_Palm;

        if (StrContains(smName, "Z_L_LegThigh")) return EnemyPieceType::LegL_Thigh;
        if (StrContains(smName, "Z_L_LegCalf"))  return EnemyPieceType::LegL_Calf;

        if (StrContains(smName, "Z_R_LegThigh")) return EnemyPieceType::LegR_Thigh;
        if (StrContains(smName, "Z_R_LegCalf"))  return EnemyPieceType::LegR_Calf;

        // Fallback
        return EnemyPieceType::Generic;
    }

    uint32_t SceneUtils::BoneForPieceType(const SkeletonAsset& skel, EnemyPieceType t)
    {
        // Skeleton names in your log: "Base HumanHead", "Base HumanSpine1", ...
        // Use "contains" keys that will match those.
        switch (t)
        {
        case EnemyPieceType::Head:       return SkeletonRegistry::FindBoneContains(skel, "Head");
        case EnemyPieceType::Torso:      return SkeletonRegistry::FindBoneContains(skel, "Spine1");
        case EnemyPieceType::Hip:        return SkeletonRegistry::FindBoneContains(skel, "Pelvis");

        case EnemyPieceType::ArmL_Upper: return SkeletonRegistry::FindBoneContains(skel, "LArmUpperarm");
        case EnemyPieceType::ArmL_Forearm:return SkeletonRegistry::FindBoneContains(skel, "LArmForearm");
        case EnemyPieceType::ArmL_Palm:  return SkeletonRegistry::FindBoneContains(skel, "LArmPalm");

        case EnemyPieceType::ArmR_Upper: return SkeletonRegistry::FindBoneContains(skel, "RArmUpperarm");
        case EnemyPieceType::ArmR_Forearm:return SkeletonRegistry::FindBoneContains(skel, "RArmForearm");
        case EnemyPieceType::ArmR_Palm:  return SkeletonRegistry::FindBoneContains(skel, "RArmPalm");

        case EnemyPieceType::LegL_Thigh: return SkeletonRegistry::FindBoneContains(skel, "LLegThigh");
        case EnemyPieceType::LegL_Calf:  return SkeletonRegistry::FindBoneContains(skel, "LLegCalf");

        case EnemyPieceType::LegR_Thigh: return SkeletonRegistry::FindBoneContains(skel, "RThigh");
        case EnemyPieceType::LegR_Calf:  return SkeletonRegistry::FindBoneContains(skel, "RCalf");

        default: break;
        }

        // Safe fallback: pelvis or root-ish bone if you have it
        uint32_t pelvis = SkeletonRegistry::FindBoneContains(skel, "Pelvis");
        if (pelvis != 0xFFFFFFFFu) return pelvis;

        // As a last resort return invalid (your detach code can handle it)
        return 0xFFFFFFFFu;
    }


}