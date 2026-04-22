#include "pch.h"

#include "Components/Player/CharacterControllerComponent.h"
#include "Components/Combat/HealthComponent.h"
#include "Components/Projectiles/ProjectileComponent.h"
#include "Components/NPC/NpcAIComponent.h"
#include "Components/Combat/WeaponComponent.h"
#include "Components/Render/RoofRenderComponent.h"
#include "Components/Vehicles/VehicleComponent.h"
#include "Components/Vehicles/DriverComponent.h"

#include "Engine/AssetManager/AssetManager.h"

#include "Engine/Map/TextureStreaming/TextureStreamingSystem.h"
#include "Components/Map/AreaComponent.h"

namespace Engine {


    entt::entity Scene::GetEntityByUUID(entt::registry& registry, Engine::UUID uuid)
    {
        auto view = registry.view<IDComponent>();

        for (auto entity : view)
        {
            auto& idComponent = view.get<IDComponent>(entity);
            if (idComponent.ID == uuid) {
                return entity;
            }
        }

        return entt::null;  // Return null if no matching entity is found
    }



    template<typename Component>
    static void CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
    {
        auto view = src.view<Component>();
        for (auto e : view)
        {
            UUID uuid = src.get<IDComponent>(e).ID;
            EE_CORE_ASSERT(enttMap.find(uuid) != enttMap.end());

            entt::entity dstEnttID = enttMap.at(uuid);

            auto& component = src.get<Component>(e);
            dst.emplace_or_replace<Component>(dstEnttID, component);

        }

    }

    template<typename Component>
    static void CopyComponentIfExists(Entity dstEntity, Entity srcEntity)
    {
        if (srcEntity.HasComponent<Component>())
        {
            dstEntity.AddOrReplaceComponent<Component>(srcEntity.GetComponent<Component>());
        }
    }

    Ref<Scene> Scene::Copy(Ref<Scene> other)
    {
        Ref<Scene> newScene = std::make_shared<Scene>();

        newScene->m_viewportWidth = other->m_viewportWidth;
        newScene->m_viewportHeight = other->m_viewportHeight;
        newScene->m_viewportBounds[0] = other->m_viewportBounds[0];
        newScene->m_viewportBounds[1] = other->m_viewportBounds[1];

        newScene->SetTextureStreamingSystem(other->GetTextureStreamingSystemRef());

        std::unordered_map<UUID, entt::entity> enttMap;

        auto& srcSceneRegistry = other->m_registry;
        auto& dstSceneRegistry = newScene->m_registry;
        auto idView = srcSceneRegistry.view<IDComponent>();
        for (auto e : idView)
        {
            UUID uuid = srcSceneRegistry.get<IDComponent>(e).ID;
            const auto& name = srcSceneRegistry.get<TagComponent>(e).Tag;
            Entity newEntity = newScene->CreateEntityWithUUID(uuid, name);

            enttMap[uuid] = (entt::entity)newEntity;

        }

        CopyAllComponents(dstSceneRegistry, srcSceneRegistry, enttMap);

        newScene->m_gameplaySystems = other->m_gameplaySystems;

       newScene->m_compactTileMap = other->m_compactTileMap;
       newScene->m_tileDefinitions = other->m_tileDefinitions;

        return newScene;
    }


    void Scene::CopyEntities(Ref<Scene> sourceScene, Ref<Scene> combinedScene, std::unordered_map<UUID, entt::entity>& enttMap)
    {
        auto& srcSceneRegistry = sourceScene->m_registry;
        auto& dstSceneRegistry = combinedScene->m_registry;
        auto idView = srcSceneRegistry.view<IDComponent>();

        std::vector<entt::entity> entitiesToCopy; // Store entities to copy components later

        for (auto e : idView)
        {
            UUID uuid = srcSceneRegistry.get<IDComponent>(e).ID;

            // If the entity already exists in enttMap, skip it
            if (enttMap.find(uuid) != enttMap.end())
                continue;

            // Otherwise, create a new entity and store it in enttMap
            const auto& name = srcSceneRegistry.get<TagComponent>(e).Tag;
            Entity newEntity = combinedScene->CreateEntityWithUUID(uuid, name);
            enttMap[uuid] = (entt::entity)newEntity;

            // Store the entity for component copying
            entitiesToCopy.push_back(e);
        }

        // Now, copy components only for the entities that were actually added
        for (auto e : entitiesToCopy)
        {
            UUID uuid = srcSceneRegistry.get<IDComponent>(e).ID;
            entt::entity dstEnttID = enttMap.at(uuid); // Now guaranteed to exist

            CopyAllComponents(dstSceneRegistry, srcSceneRegistry, enttMap);

        }
    }




    Ref<Scene> Scene::Combine(Ref<Scene> sceneA, Ref<Scene> sceneB)
    {
        Ref<Scene> combinedScene = std::make_shared<Scene>();

        // usually sceneB is the Game scene that has the TextureStreaming
        combinedScene->SetTextureStreamingSystem(sceneB->GetTextureStreamingSystemRef());



        combinedScene->m_viewportWidth = sceneA->m_viewportWidth;
        combinedScene->m_viewportHeight = sceneA->m_viewportHeight;
        combinedScene->m_viewportBounds[0] = sceneA->m_viewportBounds[0];
        combinedScene->m_viewportBounds[1] = sceneA->m_viewportBounds[1];

        std::unordered_map<UUID, entt::entity> enttMap;

        CopyEntities(sceneA, combinedScene, enttMap);
        CopyEntities(sceneB, combinedScene, enttMap);

        combinedScene->m_gameplaySystems = sceneA->m_gameplaySystems;
        combinedScene->m_gameplaySystems.insert(
            combinedScene->m_gameplaySystems.end(),
            sceneB->m_gameplaySystems.begin(),
            sceneB->m_gameplaySystems.end()
        );

        return combinedScene;
    }


    void Scene::CopyAllComponents(entt::registry& dstSceneRegistry, entt::registry& srcSceneRegistry, const std::unordered_map<UUID, entt::entity>& enttMap)
    {
        CopyComponent<TransformComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<SpriteRendererComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<CameraComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<BoxCollider2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<RigidBody2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<NativeScriptComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<CircleRendererComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<CircleCollider2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<CharacterControllerComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<HealthComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<ProjectileComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<NPCAIMovementComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<NPCAIVisionComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<WeaponComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        //CopyComponent<TileComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<RoofRenderComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<VehicleComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<DriverComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<AreaComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
    }

    Entity Scene::CreateEntity(const std::string& name)
    {

        return CreateEntityWithUUID(UUID(), name);

    }

    Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
    {
        Entity entity = { m_registry.create(), this };
        entity.AddComponent<IDComponent>(uuid);
        auto tag = entity.AddComponent<TagComponent>(std::move(name.empty() ? "Entity" : name));
        return entity;
    }

    bool Scene::DestroyEntity(Entity entity)
    {

        if (m_registry.valid(entity))
        {


            m_registry.destroy(entity);
            return true;
        }


        EE_CORE_WARN("Tried to destroy an invalid entity!");
        return false;
    }



    void Scene::DuplicateEntity(Entity entity)
    {
        Entity newEntity = CreateEntity(entity.GetName());

        CopyComponentIfExists<TransformComponent>(newEntity, entity);
        //CopyComponentIfExists<SpriteRendererComponent>(newEntity, entity);
        CopyComponentIfExists<CameraComponent>(newEntity, entity);
        CopyComponentIfExists<BoxCollider2DComponent>(newEntity, entity);
        CopyComponentIfExists<RigidBody2DComponent>(newEntity, entity);
        CopyComponentIfExists<NativeScriptComponent>(newEntity, entity);
        CopyComponentIfExists<CircleRendererComponent>(newEntity, entity);
        CopyComponentIfExists<CircleCollider2DComponent>(newEntity, entity);
        CopyComponentIfExists<TileComponent>(newEntity, entity);

        if (entity.HasComponent<SpriteRendererComponent>())
        {
            SpriteRendererComponent& spriteComp = newEntity.AddComponent<SpriteRendererComponent>();
            //spriteComp.Texture = AssetManager::CloneTexture(entity.GetComponent<SpriteRendererComponent>().Texture->GetName());
            std::vector<uint8_t> pixelData;
            std::vector<uint8_t> healthData;
            int width, height;

            if (AssetManager::GetTexturePixelData(entity.GetComponent<SpriteRendererComponent>().Texture->GetName(), pixelData, healthData, width, height))
            {
                EE_CORE_WARN("health data not implemented to GetTexturePixelData");

                m_textureStreamingSystem->UploadToChunkFromTexture(
                    entity.GetComponent<TransformComponent>().Translation,
                    entity.GetComponent<IDComponent>().ID,
                    entity.GetComponent<SpriteRendererComponent>().Texture->GetName(),
                    pixelData, healthData, width, height);

            }
        }

    }






    Entity Scene::GetPrimaryCameraEntity()
    {
        auto view = m_registry.view<CameraComponent>();
        for (auto cameraEntity : view)
        {
            const auto& cameraComp = view.get<CameraComponent>(cameraEntity);

            if (cameraComp.Primary)
            {
                return Entity{ cameraEntity, this };
            }
        }
        return {};
    }
}