#pragma once
#include <vector>
#include "glm/glm.hpp"

namespace Engine {


    enum class EnemyPieceType : uint8_t
    {
        Generic = 0,

        Head,
        Torso,
        Hip,

        ArmL_Upper,
        ArmL_Forearm,
        ArmL_Palm,

        ArmR_Upper,
        ArmR_Forearm,
        ArmR_Palm,

        LegL_Thigh,
        LegL_Calf,
        LegR_Thigh,
        LegR_Calf,

        Armor1,
        Armor2,
    };


        enum class HitVolumeShape : uint8_t {
            Sphere,
            
        };

        struct EnemyPiece {
            EnemyPieceType type;

            // Rendering / detach
            uint32_t submeshIndex;   // index into MeshAsset::submeshes
            uint32_t boneId;         // bone to use as reference when detaching (optional)

            uint8_t  visible = 1;    // 1 = render on main enemy
            uint8_t  canDetach = 1;
            uint8_t  detached = 0;    // 1 = already detached/spawned
            uint8_t  _pad0 = 0;

            // Hit volume (local/model space)
            HitVolumeShape hitShape = HitVolumeShape::Sphere;
            uint8_t        hitEnabled = 1;   // 0 = ignore for hits
            uint8_t        _pad1[2] = { 0,0 };

            glm::vec3 hitLocalCenter = glm::vec3(0.0f);  // center in local space
            float     hitRadius = 0.3f;             // for sphere
        };

        struct EnemyDestructibleComponent {
            std::vector<EnemyPiece> pieces;
        };

    

}