#pragma once
#include "Command.h"
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Components/Render/TileComponent.h>


namespace Engine {

    
    class Scene;
    
    class PlaceTileCommand : public Command {
    public:
        PlaceTileCommand(Scene* scene, Entity entity, const TileInfo& tileData, bool createdNewEntity);

        virtual void Execute() override;
        virtual void Undo() override;

    private:
        Scene* m_Scene;
        Entity m_Entity;
        TileInfo m_TileData;
        bool m_CreatedNewEntity;
    };
    

   

}