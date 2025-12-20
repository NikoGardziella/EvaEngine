#pragma once
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Components/Render/TileComponent.h>
#include <optional>
#include <Engine/Scene/Components/NPC/NpcAIComponent.h>
#include <Engine/Scene/Components/NPC/NpcAIStateComponent.h>

namespace Engine {

	class EditorUtils
	{
	public:
		static Entity FindEntityAtPosition(Ref<Scene> scene, const glm::vec2& worldPosition);
		//static Entity FindTileAtPosition(Ref<Scene> scene, const glm::vec2& worldPosition);
		static void EditorUtils::DeleteTileAtPosition(Entity entity, const glm::vec2& worldPosition);
		static std::optional<size_t> FindTileIndexAtPosition(const TileComponent& tileComp, const TransformComponent& transform, const glm::vec2& worldPos);
		static const char* AIStateToString(AIState s);
		static std::vector<std::string>& EditorUtils::GetTileNamesByCategoryAndMaterial(eTileCategory category, eTileMaterial material);


	};


}

