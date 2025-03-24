#pragma once
#include "Engine/Core/UUID.h"
#include "Engine/Scene/Component.h"
#include "Engine/Scene/Scene.h"

#include <entt.hpp>



namespace Engine {

   // class Scene;

	class Entity
	{
	public:
        Entity() = default;
        Entity(entt::entity handle, Scene* scene)
            : m_entityHandle(handle), m_scene(scene) {
        }


        template<typename T>
        bool HasComponent() const
        {
            EE_CORE_ASSERT(m_scene, "Entity has no valid scene!");
            return m_scene->m_registry.all_of<T>(m_entityHandle);
        }

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            EE_CORE_ASSERT(m_scene, "Entity has no valid scene!");
            EE_CORE_ASSERT(m_entityHandle != entt::null, "Invalid entity handle!");
            EE_CORE_ASSERT(!HasComponent<T>(), "Entity already has component");

            T& component = m_scene->m_registry.emplace<T>(m_entityHandle, std::forward<Args>(args)...);
            m_scene->OnComponentAdded<T>(*this, component);
            return component;
        }

        template<typename T, typename... Args>
        T& AddOrReplaceComponent(Args&&... args)
        {

            T& component = m_scene->m_registry.emplace_or_replace<T>(m_entityHandle, std::forward<Args>(args)...);
            m_scene->OnComponentAdded<T>(*this, component);
            return component;
        }

        template<typename T>
        T& GetComponent()
        {
            EE_CORE_ASSERT(m_scene, "Entity has no valid scene!");
            EE_CORE_ASSERT(m_entityHandle != entt::null, "Invalid entity handle!");
            EE_CORE_ASSERT(HasComponent<T>(), "Entity does not have component");
            return m_scene->m_registry.get<T>(m_entityHandle);
        }

        template<typename T>
        void RemoveComponent()
        {
            EE_CORE_ASSERT(m_scene, "Entity has no valid scene!");
            m_scene->m_registry.remove<T>(m_entityHandle);
        }

        operator entt::entity() const { return m_entityHandle; }
        operator bool() const { return m_entityHandle != entt::null; }
        operator uint32_t() const { return (uint32_t)m_entityHandle; }

        UUID GetUUID() { return GetComponent<IDComponent>().ID; }
        const std::string& GetName() { return GetComponent<TagComponent>().Tag; }

        bool operator==(const Entity& other) const
        {
            return m_entityHandle == other.m_entityHandle && m_scene == other.m_scene;
        }

        bool operator!=(const Entity& other) const
        {
            return !(*this == other);
        }

        const Scene* GetScene() { return m_scene;  }

	private:
        entt::entity m_entityHandle{ entt::null };
		Scene* m_scene = nullptr;

        friend class Scene;
	};


}

