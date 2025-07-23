#include "pch.h"
#include "GridMap.h"
#include <Engine/Scene/Components/Render/TileComponent.h>
#include <Engine/Scene/Component.h>
#include <Engine/Renderer/VulkanRenderer2D.h>
#include "Engine/Map/TextureStreaming/TextureStreamingUtils.h"
#include <glm/gtc/matrix_transform.hpp>
#include <Engine/Map/Utils/IVec2Hasher.h>
#include <unordered_set>

namespace Engine
{
	void GridMap::BuildFromRegistry(entt::registry& registry)
	{
        EE_PROFILE_FUNCTION();
		m_blockedTiles.clear();

		auto view = registry.view<TileComponent, TransformComponent>();
		for (auto entity : view)
		{
			auto [tileComp, transform] = view.get<TileComponent, TransformComponent>(entity);

			for (const auto& tile : tileComp.tiles)
			{
				if (tile.Category != eTileCategory::Buildings)
					continue;


				glm::ivec2 worldTilePos = MapUtils::GetWorldTileCoords(tile.position, transform.Translation);
				
				m_blockedTiles.insert(worldTilePos);
			}
		}
	}
    void GridMap::MarkBlockedSubtilesFromTexture(
        const glm::vec2& worldPosition, // tile position in world units (e.g., (5, 10))
        const std::vector<uint8_t>& textureData,
        uint32_t textureWidth, uint32_t textureHeight)
    {

        for (uint32_t y = 0; y < textureHeight; ++y)
        {
            for (uint32_t x = 0; x < textureWidth; ++x)
            {
                size_t index = (y * textureWidth + x) * 4;
                uint8_t alpha = textureData[index + 3];
                if (alpha < 10) continue; // skip transparent pixels

                // Pixel position in fractional tile units
                glm::vec2 pixelOffsetInTile = glm::vec2(float(x) / float(PIXELS_IN_TILE), float(y) / float(PIXELS_IN_TILE));

                // Total subtile world position
                glm::vec2 subtileWorldPos = worldPosition + pixelOffsetInTile;

                // Convert to subtile grid coordinate (integers)
                glm::ivec2 subtileGridCoord = glm::floor(subtileWorldPos * float(GRID_SUBDIVISIONS));

                m_blockedTiles.insert(subtileGridCoord);
            }
        }
    }





	bool GridMap::IsBlocked(glm::ivec2 worldTileCoords) const
	{
        EE_PROFILE_FUNCTION();
		return m_blockedTiles.find(worldTileCoords) != m_blockedTiles.end();
	}

	void GridMap::Clear()
	{
		m_blockedTiles.clear();
	}

    bool GridMap::HasLineOfSight(glm::vec2 fromWorld, glm::vec2 toWorld, bool debugDraw)
    {
        EE_PROFILE_FUNCTION();

        constexpr float subtileSize = float(TILE_SIZE) / float(GRID_SUBDIVISIONS);

        // Convert world position to subtile coordinates:
        glm::ivec2 from = glm::floor(fromWorld / subtileSize);
        glm::ivec2 to = glm::floor(toWorld / subtileSize);

        int x0 = from.x;
        int y0 = from.y;
        int x1 = to.x;
        int y1 = to.y;

        int dx = abs(x1 - x0);
        int dy = abs(y1 - y0);
        int sx = x0 < x1 ? 1 : -1;
        int sy = y0 < y1 ? 1 : -1;
        int err = dx - dy;

        bool isFirstTile = true;

        while (true)
        {
            glm::ivec2 subtileCoord = { x0, y0 };

            if (!isFirstTile && m_blockedTiles.find(subtileCoord) != m_blockedTiles.end())
            {
                if (debugDraw)
                {
                    glm::vec2 subtileCenter = glm::vec2(subtileCoord) * subtileSize + glm::vec2(subtileSize * 0.5f);
                    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(subtileCenter, 0.0f)) *
                        glm::scale(glm::mat4(1.0f), glm::vec3(subtileSize));
                    Engine::VulkanRenderer2D::DrawLineRect(model, glm::vec4(1, 0, 0, 0.3f), -1.0f);
                    DrawDebugLine(fromWorld, toWorld, glm::vec4(1, 0, 0, 1));
                }
                return false;
            }

            if (debugDraw)
            {
                glm::vec2 subtileCenter = glm::vec2(subtileCoord) * subtileSize + glm::vec2(subtileSize * 0.5f);
                glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(subtileCenter, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(subtileSize));
                Engine::VulkanRenderer2D::DrawLineRect(model, glm::vec4(1, 1, 0, 0.2f), -1.0f);
            }

            if (x0 == x1 && y0 == y1)
                break;

            int e2 = 2 * err;
            if (e2 > -dy)
            {
                err -= dy;
                x0 += sx;
            }
            if (e2 < dx)
            {
                err += dx;
                y0 += sy;
            }

            isFirstTile = false;
        }

        if (debugDraw)
        {
            glm::vec2 subtileCenter = glm::vec2(to) * subtileSize + glm::vec2(subtileSize * 0.5f);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(subtileCenter, 0.0f)) *
                glm::scale(glm::mat4(1.0f), glm::vec3(subtileSize));
            Engine::VulkanRenderer2D::DrawLineRect(model, glm::vec4(0, 1, 1, 0.4f), -1.0f);
            DrawDebugLine(fromWorld, toWorld, glm::vec4(0, 1, 0, 1));
        }

        return true;
    }






	void GridMap::DrawDebugLine(glm::vec2 from, glm::vec2 to, const glm::vec4& color)
	{
        EE_PROFILE_FUNCTION();
		glm::vec3 a(from, 0.1f); // slight Z offset
		glm::vec3 b(to, 0.1f);
		Engine::VulkanRenderer2D::DrawLine(a, b, color, -1);
	}

    void GridMap::DrawDebugBlockedTiles() const
    {
        EE_PROFILE_FUNCTION();
        constexpr float tileSize = static_cast<float>(TILE_SIZE);  
        constexpr float subtileSize = tileSize / float(GRID_SUBDIVISIONS); 

        glm::vec4 debugColor = glm::vec4(1.0f, 0.0f, 0.0f, 0.3f); // Semi-transparent red

        for (const glm::ivec2& subtilePos : m_blockedTiles)
        {
            // Calculate center position of subtile in world units
            glm::vec2 worldPos = glm::vec2(subtilePos) * subtileSize + glm::vec2(subtileSize * 0.5f);

            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(worldPos, 0.0f)) *
                glm::scale(glm::mat4(1.0f), glm::vec3(subtileSize, subtileSize, 1.0f));

            Engine::VulkanRenderer2D::DrawLineRect(model, debugColor, -1.0f);
        }
    }


}