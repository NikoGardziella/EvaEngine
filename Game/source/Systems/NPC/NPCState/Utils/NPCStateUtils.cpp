#include "NPCStateUtils.h"

    bool NPCStateUtils::IsLegPiece(Engine::EnemyPieceType t)
    {
        return t == Engine::EnemyPieceType::LegL_Calf ||
            t == Engine::EnemyPieceType::LegL_Thigh ||
            t == Engine::EnemyPieceType::LegR_Calf ||
            t == Engine::EnemyPieceType::LegR_Thigh;
    }

    bool NPCStateUtils::IsLeftLegPiece(Engine::EnemyPieceType t)
    {
        return t == Engine::EnemyPieceType::LegL_Calf;
    }

    bool NPCStateUtils::IsRightLegPiece(Engine::EnemyPieceType t)
    {
        return t == Engine::EnemyPieceType::LegR_Calf;
    }

    bool NPCStateUtils::IsArmPiece(Engine::EnemyPieceType t)
    {
        return t == Engine::EnemyPieceType::ArmL_Forearm ||
            t == Engine::EnemyPieceType::ArmL_Upper ||
            t == Engine::EnemyPieceType::ArmR_Forearm ||
            t == Engine::EnemyPieceType::ArmR_Upper;
    }

    bool NPCStateUtils::IsTorsoPiece(Engine::EnemyPieceType t)
    {
        return t == Engine::EnemyPieceType::Torso || t == Engine::EnemyPieceType::Hip;
    }


    bool NPCStateUtils::PiecePresent(const Engine::EnemyPiece& p)
    {
        return p.visible != 0;
    }

