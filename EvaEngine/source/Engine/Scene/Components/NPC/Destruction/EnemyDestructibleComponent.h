#pragma once
#include <vector>


namespace Engine {


    enum class EnemyPieceType : uint8_t {
        Head,
        Torso,
        ArmL,
        ArmR,
        LegL,
        LegR,
        Armor1,
        Armor2,
        
    };

    struct EnemyPiece {
        EnemyPieceType type;
        uint32_t submeshIndex;   // index into MeshAsset::submeshes
        uint32_t boneId;         // bone to use as reference when detaching (optional)
        uint8_t  visible = 1;    // 1 = render, 0 = hidden
        uint8_t  canDetach = 1;
        uint8_t  detached = 0;
        uint16_t _pad = 0;
    };

    struct EnemyDestructibleComponent {
        std::vector<EnemyPiece> pieces;
    };
}