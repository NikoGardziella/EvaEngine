#pragma once
#include <Engine/Renderer/Renderer2D/VulkanRenderer2D.h>
#include "TileStreaming.h"

namespace Engine {


	
	class Scene;
	class TileManager {


	public:



		void BuildInitialResidency(Scene* scene);

		uint32_t EnsureVisualResident(uint64_t uid);

		void BuildTemplatesForScene(Scene* scene);
		TileTypeKey MakeTileTypeKey(const TileInfo& tile) const;
		bool GetOriginalTileData(uint64_t uid, const std::vector<uint8_t>*& color, const std::vector<uint8_t>*& props) const;
		void ClearTemplates();
		void Update(Scene* scene, glm::vec2 playerPos);
		void Shutdown();
		void SetTileWorldSize(float w, float h)
		{
			m_tileWorldW = w; 
			m_tileWorldH = h;
		}


		
		
		

	private:



		struct ColorTemplate { int w = 0, h = 0; std::vector<uint8_t> rgba; };              // RGBA8 bottom-origin
		struct PropsTemplate { int w = 0, h = 0; std::vector<uint8_t> rgba; std::vector<uint8_t> aliveMask; uint8_t catNibble = 0; };
		// GA: interleaved G,A (2 bytes/px). aliveMask: (visible || footBand) from atlas+props.
		

		// ************************************************************************
		// for optimized tiles
		std::unordered_map<TileTypeKey, ColorTemplate, TileTypeKeyHash> m_colorByType;
		std::unordered_map<TileTypeKey, PropsTemplate, TileTypeKeyHash> m_propsByType;
		std::unordered_map<uint64_t, TileTypeKey> m_typeByUID;
		std::unordered_map<TileTypeKey, std::pair<glm::ivec2, glm::ivec2>, TileTypeKeyHash> m_opaqueByType;

		// old ones. maybe remove at some point
		std::unordered_map<uint64_t, glm::vec2> m_centerByUID; 
		std::unordered_map<uint64_t, uint32_t> m_slotByUID;
		//std::unordered_map<uint64_t, ColorTemplate> m_colorByUID;
		//std::unordered_map<uint64_t, PropsTemplate> m_propsByUID;
		// ***************************************************************************

		

		std::vector<glm::vec2> m_slotOriginWorld; 
		float m_tileWorldW = TILE_PIXEL_WIDTH;              
		float m_tileWorldH = TILE_PIXEL_HEIGHT;

		TileStreamingSystem m_streaming;
	};
}


