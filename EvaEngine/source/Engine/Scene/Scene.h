#pragma once

#include <entt.hpp>
#include "Engine/Core/Timestep.h"
#include "Engine/Renderer/EditorCamera.h"
#include "Engine/Core/UUID.h"

#include "box2d/id.h"
#include <functional>

#include "TaskManager/PhysicsTaskScheduler.h"
#include <Engine/Map/Tile/DestrutibleTileSystem.h>
#include <Engine/Animation/2D/AnimationSystem.h>
#include <Engine/Animation/2D/AnimationBank2D.h>



namespace Engine {

	class TileManager;
	class TextureStreamingSystem;
	class Entity;
	class GridMap;
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

		// justa wrapper to make cleaner entt registry query 
		template<class... Cs, class Fn>
		void ForEach(Fn&& fn)
		{
			auto view = m_registry.view<Cs...>();
			for (auto e : view)
			{
				fn(Entity{ e, this }, view.template get<Cs>(e)...); // passes refs to Cs...
			}
		}

		


		// Const variant (if you need read-only iteration)
		// (Ideally have a ConstEntity wrapper; shown here passing Entity too.)
		template<class... Cs, class Fn>
		void ForEachConst(Fn&& fn) const
		{
			auto view = m_registry.view<const Cs...>();
			for (auto e : view)
			{
				fn(Entity{ e, const_cast<Scene*>(this) }, view.template get<const Cs>(e)...);
			}
		}


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

		Ref<GridMap>& GetGrid() { return m_gridMap; }

		void ClearRegistry() { m_registry.clear(); };
		entt::registry& GetRegistry() { return m_registry;  }
		

	
		std::array<glm::vec2, 2>& GetViewportBounds() { return m_viewportBounds; }
		uint32_t GetViewortHeight() { return m_viewportHeight; }
		uint32_t GetViewportWidth() { return m_viewportWidth; }


		void RegisterSystem(const std::function<void(float, Scene*)>& system);
		void SetDebugDrawLOS(bool drawLOS) { m_debugDrawLOS = drawLOS; }

		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_registry.view<Components...>();
		}

	private:

		//template<typename T>
		//void OnComponentAdded(Entity entity, T& component);




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

		std::vector<std::function<void(float, Scene*)>> m_gameplaySystems;

		Ref<TextureStreamingSystem> m_textureStreamingSystem;
		Ref<TileManager> m_tileMananger;
		Ref<GridMap> m_gridMap;
		Ref<AnimationSystem2D> m_animationSystem;
		Ref<AnimationBank2D> m_animationBank;
		DestructibleTileSystem m_destructibleTileSystem;
		bool m_debugDrawLOS = false;

		friend class Entity;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
	};

	

}

