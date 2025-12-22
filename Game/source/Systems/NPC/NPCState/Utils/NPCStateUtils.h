#pragma once
#include <Engine/Scene/Components/NPC/Destruction/EnemyDestructibleComponent.h>


class NPCStateUtils
{
public:
	static bool IsLegPiece(Engine::EnemyPieceType t);
	static bool IsLeftLegPiece(Engine::EnemyPieceType t);
	static bool IsRightLegPiece(Engine::EnemyPieceType t);
	static bool IsArmPiece(Engine::EnemyPieceType t);
	static bool IsTorsoPiece(Engine::EnemyPieceType t);
	static bool PiecePresent(const Engine::EnemyPiece& p);
};

