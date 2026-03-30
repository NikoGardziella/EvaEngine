#pragma once
#include "TileDefinition.h"
#include "glm/glm.hpp"
#include <Engine/Scene/Scene.h>
#include <Engine/Core/Core.h>
#include "TileManager.h"


namespace Engine {

	class CompactTilePromotion
	{
	public:


		//bool PromoteTileAt(Scene* scene, const glm::ivec2& worldCell);

		Entity PromoteGroup(Scene* scene, uint64_t groupId);
		bool CompactEntityToTiles(Scene* scene, Entity entity, bool destroyOriginalEntity = true);
		void EnsurePromotedAndCompactedAroundPlayer(Scene* scene, const glm::vec2& playerWorldPos, float promoteRadiusWorld, float compactRadiusWorld, Ref<TileManager> tileMananger);
		void EnsurePromotedInEditorViewport(Scene* scene, const glm::vec2& viewMinWorld, const glm::vec2& viewMaxWorld, float compactMarginWorld, Ref<TileManager> tileManager);
		void RegisterPromotedEntity(uint64_t groupId, Entity entity);
		bool PromoteSingleTileIntoExistingGroup(Scene* scene, uint64_t groupId, const glm::ivec2& worldCell, Ref<TileManager> tileManager);
		void RegisterPromotedTile(Entity entity, TileInfo& tile);

		void RemoveSingleTileFromExistingGroup(Scene* scene, uint64_t groupId, const glm::ivec2& worldCell, Ref<TileManager> tileManager);
		
		bool IsGroupPromoted(uint64_t groupId);
	private:
		uint16_t GetOrCreateDefinitionForRuntimeTile(Scene* scene, const TileInfo& tile);
		TileInfo BuildRuntimeTileFromDefinition(const TileDefinition& def, const glm::vec2& localPos);

		void UnregisterPromotedEntity(uint64_t groupId);
		Entity GetPromotedEntity(uint64_t groupId);



	private:
		std::unordered_map<uint64_t, Entity> m_PromotedEntitiesByGroup;
	};
}

