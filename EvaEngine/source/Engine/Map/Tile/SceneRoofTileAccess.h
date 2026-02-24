#pragma once

#include <glm/glm.hpp>
#include <string>

#include "RoofSystem.h" // for IRoofTileAccess

namespace Engine {

    class Scene;
    class Entity;


    class SceneRoofTileAccess 
    {
    public:


    public:
        SceneRoofTileAccess() = default;

        void Init(Scene* scene ) { m_scene = scene;  }

        bool HasRoof(const glm::ivec2& p) const;
        bool HasSupport(const glm::ivec2& p) const;
        void RemoveRoof(const glm::ivec2& p);
        bool ApplyRoofDamage(const glm::ivec2& p, int damage);

    private:

        bool FindFirstTileAt(const glm::ivec2& p, bool wantRoof, bool wantSupport,
            Entity* outOwner, size_t* outIndex) const;

    private:
        Scene* m_scene = nullptr;
    };

}