#pragma once
#include "glm/glm.hpp"
#include <unordered_map>
#include <functional>
#include <entt.hpp>



namespace Engine {

	class Entity;
	class Scene;
	class TransformSystem3D
	{
	public:

		 // remove this class?
		void Update(Scene* scene, float dt);
		

		const glm::mat4* TryGetWorld(Entity e) const;

		void ClearCache();

	private:
		std::unordered_map<entt::entity, glm::mat4> m_cachedWorld;

	};
}



