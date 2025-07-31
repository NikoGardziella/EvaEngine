#pragma once

#include <Engine/Scene/Scene.h>
#include <Engine/Platform/Vulkan/VulkanTexture.h>
#include <Engine/Scene/Components/Render/TileComponent.h>
#include <imgui/imgui.h>
#include <Engine/AssetManager/Utils/TileSerializer.h>


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

		void DrawTilePalette(eTileCategory category);

		void DrawTiles(const std::vector<std::string>& tileNames, ImTextureID textureID, eTileCategory category, eTileMaterial material);

		eTileCategory GetSelectedTileCategory() const { return m_selectedTileCategory; }
		TileProperties& GetSelectedTileProperties() { return m_selectedTileprops; }

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
		//void CreateTileAtlas();
	private:

		bool m_showChunks = false;
		//EditorCamera& m_editorCamera;

		//  top-left and bottom-right corners in normalized UV space.
		std::unordered_map<std::string, glm::vec4> m_tileUVMap;
		std::vector<std::string> m_tileNames;

		Ref<VulkanTexture> m_tileTextureIconAtlas;
		uint32_t m_selectedTile = UINT32_MAX; 
		std::string m_selectedTileName;
		eTileCategory m_selectedTileCategory;
		eTileMaterial m_selectedTileMaterial;

		TileProperties& m_selectedTileprops = TileProperties{};

		std::unordered_map<eTileCategory, eTileMaterial> m_selectedMaterials;

		

		std::unordered_map<std::string, TileProperties> m_tilePropertyDefaults; // key = tile name
		std::unordered_map<std::string, TileProperties> m_modifiedTileProperties; // optional overrides


	};
}



