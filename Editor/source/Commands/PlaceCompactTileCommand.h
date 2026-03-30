#pragma once

#include <glm/glm.hpp>
#include "Engine/Map/Tile/CompactTileMap.h"
#include "Command.h"

namespace Engine
{
    class Scene;

    class PlaceCompactTileCommand : public Command
    {
    public:
        PlaceCompactTileCommand(Scene* scene, const glm::ivec2& worldCell, const CompactTile& newTile);

        virtual void Execute() override;
        virtual void Undo() override;

    private:
        Scene* m_Scene = nullptr;
        glm::ivec2 m_WorldCell{};

        CompactTile m_NewTile{};
        CompactTile m_OldTile{};

        bool m_HadOldTile = false;
    };
}