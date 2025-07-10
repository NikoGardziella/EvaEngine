#pragma once
#include "Engine/Core/Timestep.h"
#include "Engine/Renderer/EditorCamera.h"
#include "Engine/Core/UUID.h"

#include "box2d/id.h"
#include "entt.hpp"

#include <functional>

#include "TaskManager/PhysicsTaskScheduler.h"
#include <Engine/Map/TextureStreaming/TextureStreamingSystem.h>



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
		void OnUpdateECSRuntime(Timestep timestep);
		void OnUpdateEditor(Timestep timestep, EditorCamera& camera);
		void OnViewportResize(uint32_t width, uint32_t height, std::array<glm::vec2, 2> viewportBounds);

		void DuplicateEntity(Entity entity);

		Entity GetPrimaryCameraEntity();
		TextureStreamingSystem& GetTextureStreamingSystem() { return *m_textureStreamingSystem; }


		void SetTextureStreamingSystem(Engine::Ref<TextureStreamingSystem>& textureStreamingSystem)
		{
			m_textureStreamingSystem = textureStreamingSystem;
		}
		Engine::Ref < TextureStreamingSystem>& GetTextureStreamingSystemRef()
		{
			return m_textureStreamingSystem;
		}

		/*
			Engine::Scope<TextureStreamingSystem> ReleaseTextureStreamingSystem()
			{
				return std::move(m_textureStreamingSystem);
			}

		*/
		void ClearRegistry() { m_registry.clear(); };

		entt::registry& GetRegistry() { return m_registry;  }
		

	
		std::array<glm::vec2, 2>& GetViewportBounds() { return m_viewportBounds; }
		uint32_t GetViewortHeight() { return m_viewportHeight; }
		uint32_t GetViewportWidth() { return m_viewportWidth; }


		void RegisterSystem(const std::function<void(entt::registry&, float, Scene*)>& system);

		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_registry.view<Components...>();
		}

	private:

		//template<typename T>
		//void OnComponentAdded(Entity entity, T& component);

		void UpdatePhysics(Timestep timestep);



	private:

		entt::registry m_registry;
		uint32_t m_viewportWidth = 0;
		uint32_t m_viewportHeight = 0;
		std::array<glm::vec2, 2> m_viewportBounds = { glm::vec2(0, 0), glm::vec2(1, 1) };


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

		std::vector<std::function<void(entt::registry&, float, Scene*)>> m_gameplaySystems;

		Engine::Ref<TextureStreamingSystem> m_textureStreamingSystem;


		friend class Entity;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
	};

	

}

