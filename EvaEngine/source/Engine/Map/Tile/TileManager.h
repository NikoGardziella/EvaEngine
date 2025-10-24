#pragma once
#include <Engine/Renderer/VulkanRenderer2D.h>

namespace Engine {


	
	class Scene;
	class TileManager {

		struct ContentRect { int x, y, w, h; }; // x,y are TOP-LEFT in tile pixels

	public:



		void BuildInitialResidency(Scene* scene);

		void BuildTemplatesForScene(Scene* scene);
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
		
	private:


		static ContentRect ComputeOpaqueBounds(const std::vector<uint8_t>& rgba, int w, int h, uint8_t alphaMin = 1)
		{
			EE_PROFILE_FUNCTION();

			int minx = w, miny = h, maxx = -1, maxy = -1;
			for (int y = 0; y < h; ++y)
			{
				for (int x = 0; x < w; ++x)
				{
					const uint8_t a = rgba[(y * w + x) * 4 + 3];
					if (a >= alphaMin)
					{
						minx = std::min(minx, x); miny = std::min(miny, y);
						maxx = std::max(maxx, x); maxy = std::max(maxy, y);
					}
				}
			}
			if (maxx < minx) return { 0,0,0,0 }; // empty
			return { minx, miny, maxx - minx + 1, maxy - miny + 1 };
		}


		static inline glm::vec2 BottomLeftFromCenter(const glm::vec2& center, float w, float h)
		{
			return glm::vec2(center.x - 0.5f * w, center.y - 0.5f * h);
		}

		struct ColorTemplate { int w = 0, h = 0; std::vector<uint8_t> rgba; };              // RGBA8 bottom-origin
		struct PropsTemplate { int w = 0, h = 0; std::vector<uint8_t> rgba; std::vector<uint8_t> aliveMask; uint8_t catNibble = 0; };
		// GA: interleaved G,A (2 bytes/px). aliveMask: (visible || footBand) from atlas+props.
		std::unordered_map<uint64_t, glm::vec2> m_centerByUID; 
		std::unordered_map<uint64_t, ColorTemplate> m_colorByUID;
		std::unordered_map<uint64_t, PropsTemplate> m_propsByUID;
		std::vector<glm::vec2> m_slotOriginWorld; 
		float m_tileWorldW = TILE_PIXEL_WIDTH;              
		float m_tileWorldH = TILE_PIXEL_HEIGHT;
	};
}


