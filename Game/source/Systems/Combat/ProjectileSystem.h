#pragma once
#include <Engine/Scene/Components/NPC/Destruction/EnemyDestructibleComponent.h>
#include <Engine/Scene/Scene.h>


class Scene;
class Entity;
class ProjectileSystem
{
	public: 
		static void UpdateProjectileSystem(float deltaTime, Engine::Scene* scen);


private:
		static void DetachPiece(Engine::Scene* scene, Engine::Entity enemy, Engine::EnemyPieceType type, const glm::vec3& impulseDir, float impulseStrength);
		
		
		static Engine::EnemyPiece* FindPiece(Engine::EnemyDestructibleComponent& destr, Engine::EnemyPieceType type);
};

