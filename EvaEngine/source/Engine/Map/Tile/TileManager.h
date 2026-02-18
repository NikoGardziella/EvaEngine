#pragma once
#include <Engine/Renderer/Renderer2D/VulkanRenderer2D.h>
#include "TileStreaming.h"

namespace Engine {


	
	class Scene;
	class TileManager {


	public:



		void BuildInitialResidency(Scene* scene);

		void BuildTemplatesForScene(Scene* scene);
		void ClearTemplates();
		void Update(Scene* scene, glm::vec2 playerPos);
		void Shutdown();
		void SetTileWorldSize(float w, float h)
		{
			m_tileWorldW = w; 
			m_tileWorldH = h;
		}


		// per-slot origin (bottom-left of the tile in world space)
		void SetSlotOriginWorld(uint32_t slot, const glm::vec2& origin)
		{
			if (slot < m_slotOriginWorld.size())
			{
				
				m_slotOriginWorld[slot] = origin;
			}
		}
		glm::vec2 GetSlotOriginWorld(uint32_t slot) const
		{
			if (slot < m_slotOriginWorld.size()) return m_slotOriginWorld[slot];
			return glm::vec2(0.0f);
		}

		uint32_t TileManager::GetSlotForUID(uint64_t uid) const
		{
			auto it = m_slotByUID.find(uid);
			if (it == m_slotByUID.end())
			{
				EE_CORE_WARN("TileManager::GetSlotForUID: UID {:016x} has no slot", uid);
				return 0xFFFFFFFFu; // sentinel for "not found"
			}
			return it->second;
		}
		
		bool GetOriginalTileData(uint64_t uid,
			const std::vector<uint8_t>*& outColor,
			const std::vector<uint8_t>*& outProps) const
		{
			auto cit = m_colorByUID.find(uid);
			auto pit = m_propsByUID.find(uid);
			if (cit == m_colorByUID.end() || pit == m_propsByUID.end())
				return false;
			outColor = &cit->second.rgba;
			outProps = &pit->second.rgba;
			return true;
		}

	private:



		struct ColorTemplate { int w = 0, h = 0; std::vector<uint8_t> rgba; };              // RGBA8 bottom-origin
		struct PropsTemplate { int w = 0, h = 0; std::vector<uint8_t> rgba; std::vector<uint8_t> aliveMask; uint8_t catNibble = 0; };
		// GA: interleaved G,A (2 bytes/px). aliveMask: (visible || footBand) from atlas+props.
		std::unordered_map<uint64_t, glm::vec2> m_centerByUID; 
		std::unordered_map<uint64_t, uint32_t> m_slotByUID;

		std::unordered_map<uint64_t, ColorTemplate> m_colorByUID;
		std::unordered_map<uint64_t, PropsTemplate> m_propsByUID;
		std::vector<glm::vec2> m_slotOriginWorld; 
		float m_tileWorldW = TILE_PIXEL_WIDTH;              
		float m_tileWorldH = TILE_PIXEL_HEIGHT;

		TileStreamingSystem m_streaming;
	};
}


