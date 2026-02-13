#include "PlaceTileCommand.h"

#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Entity.h"

 
 namespace Engine {

    PlaceTileCommand::PlaceTileCommand(Scene* scene, Entity entity, const TileInfo& tileData, bool createdNewEntity)
        : m_Scene(scene), m_Entity(entity), m_TileData(tileData), m_CreatedNewEntity(createdNewEntity)
    {
    }

    void PlaceTileCommand::Execute()
    {
        // Redo logic: Push the tile back into the component
        if (m_Entity)
        {
            TileComponent& tc = m_Entity.GetComponent<TileComponent>();

            tc.tiles.push_back(m_TileData);
        }
    }

    void PlaceTileCommand::Undo()
    {
        if (m_CreatedNewEntity)
        {
            m_Scene->DestroyEntity({ m_Entity, m_Scene });
        }
        else if (m_Scene->GetRegistry().valid(m_Entity))
        {
            auto& tc = m_Scene->GetRegistry().get<TileComponent>(m_Entity);

            // Find the tile by its unique ID and remove it
            auto it = std::find_if(tc.tiles.begin(), tc.tiles.end(),
                [this](const TileInfo& t)
                {
                    return t.UID == m_TileData.UID;
                });

            if (it != tc.tiles.end()) 
            {
                tc.tiles.erase(it);
            }
        }
    }

   
}

