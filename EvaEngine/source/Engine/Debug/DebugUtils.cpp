#include "pch.h"
#include "DebugUtils.h"
// DebugUtils.h
#pragma once

#include "entt.hpp"
#include "Engine/Core/Log.h"
#include <Engine/Scene/Component.h>
#include "Engine/Scene/Entity.h"

namespace Engine {

    void DebugUtils::LogAllEntitiesWithTags(const Ref<Scene>& scene)
    {
        auto view = scene->GetAllEntitiesWith<TagComponent>();
        EE_CORE_INFO("Entities in scene:");
        for (auto entityID : view)
        {
            Entity entity = Entity{ entityID, scene.get() };
            auto& tag = entity.GetComponent<TagComponent>().Tag;
            EE_CORE_INFO("Entity ID: {}, Tag: {}", (uint32_t)entity, tag.c_str());
        }
    }

    void DebugUtils::LogAllEntitiesWithComponents(const Ref<Scene>& scene)
    {
        EE_CORE_INFO("Logging all entities and their components:");
        auto view = scene->GetAllEntitiesWith<TagComponent>();

        for (auto entityID : view)
        {
                Entity entity{ entityID, scene.get() };

                if (entity.HasComponent<TagComponent>())
                {
                    const auto& tag = entity.GetComponent<TagComponent>().Tag;
                    EE_CORE_INFO("Entity: {}", tag.c_str());
                }
                else
                {
                    EE_CORE_INFO("Entity ID: {}", (uint32_t)entity);
                }

                EE_CORE_INFO("  Components:");

                if (entity.HasComponent<TransformComponent>())    EE_CORE_INFO("    - TransformComponent");
                if (entity.HasComponent<SpriteRendererComponent>()) EE_CORE_INFO("    - SpriteRendererComponent");
                if (entity.HasComponent<CameraComponent>())       EE_CORE_INFO("    - CameraComponent");
                if (entity.HasComponent<RigidBody2DComponent>())    EE_CORE_INFO("    - RigidBodyComponent");

                // Add more components as needed
        }
    }

    /*

    template<typename T>
    void DebugUtils::LogAllEntitiesWithComponent(const Ref<Scene>& scene, const std::string& componentName)
    {
        auto view = scene->GetAllEntitiesWith<T>();
        EE_CORE_INFO("Entities with component: {0}", componentName);
        for (auto entityID : view)
        {
            Entity entity = Entity{ entityID, scene.get() };
            EE_CORE_INFO("Entity ID: {0}", entityID);
        }
    }
    */
}

