#include "pch.h"
#include "Scene.h"

#include <glm/glm.hpp>
#include "Component.h"

#include "Engine.h"

namespace Engine {

    Scene::Scene()
    {


        
    }


    Scene::~Scene()
    {
         
    }
    Entity Scene::CreateEntity(const std::string& name)
    {

        Entity entity = { m_registry.create(), this };
        auto tag = entity.AddComponent<TagComponent>(std::move(name.empty() ? "Entity" : name));
        return entity;

    }
    void Scene::DestroyEntity(Entity entity)
    {
        m_registry.destroy(entity);
    }

    void Scene::OnUpdateRuntime(Timestep timestep)
    {


        // update scripts
        {
            m_registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
                {

                    if (!nsc.Instance)
                    {
                        nsc.Instance = nsc.InstantiateScript();
                        nsc.Instance->m_entity = Entity{ entity, this };
                        nsc.Instance->OnCreate();
                    }

                    nsc.Instance->OnUpdate(timestep);

                });
        }
        
        Camera* mainCamera = nullptr;
        glm::mat4 cameraTransform;
        {
            auto group = m_registry.group<TransformComponent, CameraComponent>();
            for (auto entity : group)
            {
                auto [transform, camera] = group.get<TransformComponent, CameraComponent>(entity);

                if (camera.Primary)
                {
                    mainCamera = &camera.Camera;
                    cameraTransform = transform.GetTransform();
                    break;
                }
            }
        }

        if(mainCamera)
        {   
            Renderer2D::BeginScene(mainCamera->GetViewProjection(), cameraTransform);
            auto group = m_registry.group<SpriteRendererComponent>(entt::get<TransformComponent>);

            for (auto entity : group)
            {
                auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);
                

                Renderer2D::DrawQuad(transform.GetTransform(), sprite.Color);
            }

            Renderer2D::EndScene();
        }


    }

    void Scene::OnUpdateEditor(Timestep timestep, EditorCamera& camera)
    {
        Renderer2D::BeginScene(camera);
        auto group = m_registry.group<SpriteRendererComponent>(entt::get<TransformComponent>);

        for (auto entity : group)
        {
            auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);


            Renderer2D::DrawQuad(transform.GetTransform(), sprite.Color);
        }

        Renderer2D::EndScene();
    }

    void Scene::OnViewportResize(uint32_t width, uint32_t height)
    {
        m_viewportHeight = height;
        m_viewportWidth = width;

        auto view = m_registry.view<CameraComponent>();

        for (auto entity : view)
        {
            auto cameraComp = view.get<CameraComponent>(entity);
            if (!cameraComp.FixedAspectRatio)
            {
                cameraComp.Camera.SetViewportSize(width, height);
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




    template<typename T>
    inline void Scene::OnComponentAdded(Entity entity, T& component)
    {
        //  fails at compile-time if there’s no explicit specialization for a given component type.
        static_assert(false);

    }

    // Specializations for Specific Components
    template<>
    void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
    {

    }

    template<>
    void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
    {
        component.Camera.SetViewportSize(m_viewportWidth, m_viewportHeight);
    }

    template<>
    void Scene::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& component)
    {

    }

    template<>
    void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component)
    {

    }

    template<>
    void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
    {

    }
}
