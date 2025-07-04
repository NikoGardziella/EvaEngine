#pragma once

#include <Engine/Scene/Scene.h>
#include <Engine/Platform/Vulkan/VulkanTexture.h>


namespace Engine
{

	class TileEditorPanel
	{
		struct TileAtlas {
			std::shared_ptr<VulkanTexture> atlasTexture;
			std::vector<glm::vec4> uvs; // (u0,v0,u1,v1) for each tile, normalized [0,1]
		};

	public:

		TileEditorPanel();
		~TileEditorPanel() = default;
		void OnImGuiRender();



		uint32_t GetSelectedTile() const { return m_selectedTile; }
		std::string GetSelectedTileName() const { return m_selectedTileName; }
		void SetSelectedTile(uint32_t tileIndex, const std::string& tileName)
		{
			m_selectedTile = tileIndex;
			m_selectedTileName = tileName;
		}

		glm::vec4 GetTileUV(const std::string& tileName) const
		{
			auto it = m_tileUVMap.find(tileName);
			if (it != m_tileUVMap.end())
			{
				return it->second;
			}
			EE_CORE_WARN("Tile UV not found for name: {}", tileName);
			return glm::vec4(0.0f); // Return empty UV if not found
		}
	private:
		void DrawTilePalette();
		void CreateTileAtlas();
	private:

		bool m_showChunks = false;
		//EditorCamera& m_editorCamera;

		//  top-left and bottom-right corners in normalized UV space.
		std::unordered_map<std::string, glm::vec4> m_tileUVMap;


		Ref<VulkanTexture> m_tileTextureIconAtlas;
		uint32_t m_selectedTile = UINT32_MAX; 
		std::string m_selectedTileName;
		std::vector<std::string> m_tileNames;
	};
}



