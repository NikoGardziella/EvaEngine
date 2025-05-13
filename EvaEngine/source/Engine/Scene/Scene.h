#pragma once
#include "Engine/Core/Timestep.h"
#include "Engine/Renderer/EditorCamera.h"
#include "Engine/Core/UUID.h"
#include "Engine/Scene/Component.h"

#include <box2d/box2d.h>
#include "box2d/id.h"
#include "entt.hpp"
#include "TaskScheduler.h"
#include <LockLessMultiReadPipe.h>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <box2d/types.h>
#include <future>

#include <box2d/box2d.h>
#include "TaskManager/PhysicsTaskScheduler.h"



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
		static void CopyAllComponents(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap);



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
		void OnViewportResize(uint32_t width, uint32_t height, glm::vec2 viewportBounds);

		void DuplicateEntity(Entity entity);

		Entity GetPrimaryCameraEntity();

		void ClearRegistry() { m_registry.clear(); };

		entt::registry& GetRegistry() { return m_registry;  }
		

		glm::vec2 GetViewportMinBounds() const { return m_viewportBounds[0]; }
		void RegisterSystem(const std::function<void(entt::registry&, float)>& system);

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
		glm::vec2 m_viewportBounds[2] = { { 0.0f, 0.0f }, { 1.0f, 1.0f } };


		b2WorldId m_worldId;
		//Structure of arrays
		struct RenderSOA
		{
			std::vector<glm::mat4> InstanceTransforms;
			std::vector<glm::vec4> Color;
			std::vector<b2BodyId> BodyIds;
		};
		RenderSOA m_renderSOA;

		PhysicsTaskScheduler m_physicsTaskScheduler;

		std::vector<std::function<void(entt::registry&, float)>> m_gameplaySystems;

		friend class Entity;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
	};

	

}

