#pragma once
#include <Engine/Scene/Components/Render/TileComponent.h>
#include "DetachedPieceTag.h"


namespace Engine
{
	class DestrutibleTileSystem
	{


	public:
		void EvaluateDetachForTile(const TileComponent& tile, const DestructibleTileDef& def);
	};

}

