#pragma once
#include "Engine/Core/Timestep.h"
#include "Engine/Renderer/EditorCamera.h"
#include "Engine/Core/UUID.h"


#include "box2d/id.h"
#include "entt.hpp"

namespace Engine {

	class Entity;
	

	class Scene
	{

	public:

		Scene();
		~Scene();

		static Ref<Scene> Copy(Ref<Scene> other);
		static Ref<Scene> Combine(Ref<Scene> sceneA, Ref<Scene> sceneB);
		static void CopyEntities(Ref<Scene> sourceScene, Ref<Scene> combinedScene, std::unordered_map<UUID, entt::entity>& enttMap);


		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
		bool DestroyEntity(Entity entity);
		static entt::entity GetEntityByUUID(entt::registry& registry, UUID uuid);

		void OnRunTimeStart();
		void OnRunTimeStop();

		void PauseRuntime();
		void ResumeRuntime();

		void OnUpdateRuntime(Timestep timestep, bool isPlaying = true);
		void OnUpdateEditor(Timestep timestep, EditorCamera& camera);
		void OnViewportResize(uint32_t width, uint32_t height);

		void DuplicateEntity(Entity entity);

		Entity GetPrimaryCameraEntity();

		void ClearRegistry() { m_registry.clear(); };

		entt::registry& GetRegistry() { return m_registry;  }

		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_registry.view<Components...>();
		}

	private:

		template<typename T>
		void OnComponentAdded(Entity entity, T& component);

		void UpdatePhysics(Timestep timestep);

	private:

		entt::registry m_registry;
		uint32_t m_viewportWidth = 0;
		uint32_t m_viewportHeight = 0;


		b2WorldId m_worldId;

		friend class Entity;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
	};

	

}

