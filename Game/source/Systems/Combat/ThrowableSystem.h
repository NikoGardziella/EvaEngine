#pragma once
#include <Engine/Scene/Components/Combat/ThrowableComponent.h>
#include <Engine/Core/Core.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Component.h>

class ThrowableSystem
{
public:
	static void UpdateThrowableSystem(float dt, Engine::Scene* scene);
	static float RandomRange(float minVal, float maxVal);
private:
	static void TriggerThrowableExplosion(Engine::Scene* scene, const Engine::TransformComponent& tr, const ThrowableComponent& g);
	static void BounceAgainstWorld(Engine::Scene* scene, const glm::vec2& oldPos, glm::vec2& newPos, ThrowableComponent& g);
	static bool IsBlocked(Engine::Ref<Engine::GridMap> grid, const glm::vec2& posWS, glm::vec2& outNormal);
};

