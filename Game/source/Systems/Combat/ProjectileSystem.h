#pragma once
#include <Engine/Scene/Components/NPC/Destruction/EnemyDestructibleComponent.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Components/Projectiles/ProjectileComponent.h>


class Scene;
class Entity;
class ProjectileSystem
{
	public: 
		static void UpdateProjectileSystem(float deltaTime, Engine::Scene* scen);


private:
		static void DetachPiece(Engine::Scene* scene, Engine::Entity enemy, Engine::EnemyPieceType type, const glm::vec3& impulseDir, float impulseStrength);

    static void IntegrateMovement(ProjectileComponent& projectileComp, Engine::TransformComponent& transformComp, float deltaTime);
    static bool IsInBlastRadius(const glm::vec2& projectilePos, const ProjectileComponent& projectileComp, Engine::Entity targetEntity, Engine::TransformComponent& targetTransform);

    static void ApplyBlastToEntity(Engine::Scene* scene, const glm::vec2& projectilePos, 
        const ProjectileComponent& projectileComp, Engine::Entity targetEntity, Engine::TransformComponent& targetTransform);

    static Engine::EnemyPieceType FindClosestPiece(const glm::vec2& projectilePos, const ProjectileComponent& projectileComp,
        Engine::Entity targetEntity, Engine::TransformComponent& targetTransform);

    static void ApplyImpulse(Engine::Entity targetEntity, const glm::vec3& impulseDir, float impulseStrength);
		
	static Engine::EnemyPiece* FindPiece(Engine::EnemyDestructibleComponent& destr, Engine::EnemyPieceType type);
};

