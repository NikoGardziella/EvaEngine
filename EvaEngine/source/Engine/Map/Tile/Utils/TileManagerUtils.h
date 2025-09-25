#pragma once
#include <Engine/Scene/Components/Render/TileComponent.h>
#include <Engine/AssetManager/Utils/TileSerializer.h>


namespace Engine {

	class TileManagerUtils
	{
	public:
		static void BuildPropsForInstance(const TileInfo& ti, const TileProperties& props, const TileSource& src, std::vector<uint8_t>& outPropsRGBA8UI);
	};
}




