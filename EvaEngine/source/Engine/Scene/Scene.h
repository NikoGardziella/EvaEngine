#pragma once

// Standard library - keep these
#include <entt.hpp>
#include "Engine/Core/Timestep.h"
#include "Engine/Core/UUID.h"
#include "box2d/id.h"
#include <functional>
#include <array>
#include <vector>
#include <string>
#include <unordered_map>

#include <Engine/Map/Tile/DestrutibleTileSystem.h>
#include <Engine/Animation/3D/System/RenderSystem3D.h> 
#include <Engine/Animation/3D/System/AnimationSystem3D.h> 
#include <Engine/Animation/3D/BonePaletteBuffer.h>    
#include "Light/LightGatherSystem.h"
#include "Physics/Box2DPhysicsSystem.h"
#include "Components/Render/TileComponent.h"
#include <Engine/Map/Tile/CompactTileMap.h>
#include <Engine/Map/Tile/TileDefinitionRegistry.h>
#include <Engine/Map/Tile/CompactTilePromotion.h>


namespace Engine {

	class EditorCamera;
	class PhysicsTaskScheduler;
	class DestructibleTileSystem;
	class AnimationSystem2D;
	class AnimationBank2D;
	class RenderSystem3D;
	class TransformSystem3D;
	class CullingSystem3D;
	class AnimationSystem3D;
	class BonePaletteBuffer;
	class MeshRegistry;
	class UIContext;
	class FogOfWar;
	class TileManager;
	class TextureStreamingSystem;
	class Entity;
	class GridMap;
	struct MeshAsset;
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

		void SpawnPlayer();

		void CreateLotsOfCompactTilesOnStartup(uint16_t typeId, int width, int height, const glm::ivec2& startIsoCell, uint64_t groupId);


		void OnRunTimeStart();
		void OnRunTimeStop();

		void PauseRuntime();
		void ResumeRuntime();

		void RenderGameUI(UIContext& ui);


		void OnUpdateRuntime(Timestep timestep, bool isPlaying = true);
		void OnUpdateECSRuntime(Timestep timestep);
		void OnUpdateEditor(Timestep timestep, EditorCamera& camera, int16_t activeFloor, bool showAllFloors);
		void ResetCompactTilePromotionState();
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

		template<typename T>
		T* TryGet(Entity e) 
		{			
			 return m_registry.try_get<T>(e);
			
		}

		template<typename T>
		T& Get(Entity e)
		{
			return m_registry.get<T>(e);
			
		}

		
		bool Scene::IsEntityValid(entt::entity entityHandle) const
		{
			
			return m_registry.valid(entityHandle);
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
		Box2DPhysicsSystem& GetBox2DPhysicsSystem() { return m_box2DPhysicsSystem; }

		void ClearRegistry() { m_registry.clear(); };
		entt::registry& GetRegistry() { return m_registry;  }
		
		CompactTileMap& GetCompactTileMap() { return m_compactTileMap; }
		CompactTilePromotion& GetCompactTilePromotion() { return m_compactTilePromotion; }
		const Ref<TileManager> GetTileManager() const { return  m_tileMananger; }

		std::array<glm::vec2, 2>& GetViewportBounds() { return m_viewportBounds; }
		uint32_t GetViewortHeight() { return m_viewportHeight; }
		uint32_t GetViewportWidth() { return m_viewportWidth; }
		float GetDeltatime() { return m_deltatime;  }

		void RegisterSystem(const std::function<void(float, Scene*)>& system);
		void SetDebugDrawLOS(bool drawLOS) { m_debugDrawLOS = drawLOS; }
		void SetDebugShowRoofs(bool drawRoof) { m_debugShowRoof = drawRoof; }
		void SetDebugShowWalls(bool drawWalls) { m_debugShowWalls = drawWalls; }

		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_registry.view<Components...>();
		}

	private:

		//template<typename T>
		//void OnComponentAdded(Entity entity, T& component);

		


	private:

		float m_deltatime = 0.0f;
		entt::registry m_registry;
		uint32_t m_viewportWidth = 0;
		uint32_t m_viewportHeight = 0;
		std::array<glm::vec2, 2> m_viewportBounds = { glm::vec2(0, 0), glm::vec2(1, 1) };


		std::vector<std::function<void(float, Scene*)>> m_gameplaySystems;

		//3D render
		Ref<TransformSystem3D> m_transformSystem3D;
		Ref<CullingSystem3D> m_cullingSystem3D;
		RenderSystem3D m_renderSystem3D;
		AnimationSystem3D m_animationSystem3D;
		BonePaletteBuffer m_bonePaletteBuffer;

		Ref<TextureStreamingSystem> m_textureStreamingSystem;
		Ref<TileManager> m_tileMananger;
		Ref<GridMap> m_gridMap;
		Ref<FogOfWar> m_fogOfWar;
		Ref<AnimationSystem2D> m_animationSystem;
		Ref<AnimationBank2D> m_animationBank;
		DestructibleTileSystem m_destructibleTileSystem;
		LightGatherSystem m_lightGatherSystem;
		Box2DPhysicsSystem m_box2DPhysicsSystem;

		bool m_debugDrawLOS = false;
		bool m_debugShowWalls = true;
		bool m_debugShowRoof = true;

		CompactTileMap m_compactTileMap;
		CompactTilePromotion m_compactTilePromotion;


		friend class Entity;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
	};

	

}

