#pragma once


#include "Engine/Core/Timestep.h"
#include "Component.h"

namespace Engine {

	class Entity;

	class Scene
	{

	public:

		Scene();
		~Scene();

		Entity CreateEntity(const std::string& name = std::string());
		void DestroyEntity(Entity entity);

		void OnUpdate(Timestep timestep);
		void OnViewportResize(uint32_t width, uint32_t height);

	private:

		template<typename T>
		void OnComponentAdded(Entity entity, T& component);


	private:

		entt::registry m_registry;
		uint32_t m_viewportWidth = 0;
		uint32_t m_viewportHeight = 0;


		friend class Entity;
		friend class SceneHierarchyPanel;
	};

	
}

