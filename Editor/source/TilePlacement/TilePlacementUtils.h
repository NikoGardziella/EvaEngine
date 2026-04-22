#pragma once
#include <string>
#include "eRectCellKind.h"
#include <Engine/Scene/Components/Render/TileComponent.h>
#include <glm/fwd.hpp>

namespace Engine {


    class TilePlacementUtils {

    public:
        static std::string RemoveExtension(const std::string& name);

        static bool SplitDirectionalTileName(const std::string& rawName, std::string& outBaseName, char& outDir);
        static std::string MakeDirectionalTileName(const std::string& baseName, char dir);

        static eTileDirection GetDirectionForRectCell(eRectCellKind kind);

        static eRectCellKind ClassifyRectangleCell(const glm::ivec2& cell, const glm::ivec2& minCell, const glm::ivec2& maxCell);

    };
}