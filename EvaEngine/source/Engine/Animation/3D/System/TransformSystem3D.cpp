#include "pch.h"
#include "TransformSystem3D.h"
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Animation/3D/Utils/AnimationUtils.h>
#include <Engine/Scene/Scene.h>
#include "glm/glm.hpp"


namespace Engine {


	void TransformSystem3D::Update(Scene* scene, float dt)
	{
        EE_PROFILE_FUNCTION();


        scene->ForEach<TransformComponent>([&](Entity e, TransformComponent& transformComp) {
            

            glm::mat4 world = AnimationUtils::TRS(transformComp.Translation, transformComp.Rotation, transformComp.Scale);
            
            m_cachedWorld[e] = world;
            });
	}

    const glm::mat4* TransformSystem3D::TryGetWorld(Entity e) const 
    {
        auto it = m_cachedWorld.find(e.Handle());
        return it == m_cachedWorld.end() ? nullptr : &it->second;
    }

    void TransformSystem3D::ClearCache()
    {
        m_cachedWorld.clear();
    }


}