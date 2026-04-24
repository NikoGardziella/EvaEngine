#pragma once
#include "Engine/Scene/Components/Render/TileComponent.h"
#include "Command.h"

#include <vector>
#include <unordered_set>
#include <glm/glm.hpp>

namespace Engine
{
    class Scene;

    class PlaceCompactTilesCommand : public Command
    {
    public:
        struct Placement
        {
            glm::ivec2 WorldCell{};
            CompactTile NewTile{};
        };

    public:
        PlaceCompactTilesCommand(Scene* scene, std::vector<Placement> placements);

        void Execute() override;
        void Undo() override;

        bool IsEmpty() const { return m_placements.empty(); }

    private:
        void RefreshTouchedGroups();
        void RefreshTouchedGroupsAfterUndo();

    private:
        Scene* m_Scene = nullptr;
        std::vector<Placement> m_placements;

        std::vector<size_t> m_executedIndices;

        std::unordered_set<uint64_t> m_touchedGroups;
    };
}