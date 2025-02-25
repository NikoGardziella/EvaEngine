#include "pch.h"
#include "Scene.h"

#include <glm/glm.hpp>
#include "Component.h"

#include "Engine.h"

namespace Engine {

    Scene::Scene()
    {


        /*
        TransformComponent compTransform;

        entt::entity entity = m_registry.create();

        m_registry.emplace<TransformComponent>(entity, glm::mat4(1.0f));

        TransformComponent& compTransform2 = m_registry.get<TransformComponent>(entity);

        // Less cache-efficient because data may be scattered in memory.
        m_registry.view<TransformComponent, SpriteRendererComponent>().each([&](auto entity, TransformComponent& transform, SpriteRendererComponent& meshComponent)
            {

            });

        auto group = m_registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);


        for (auto entity : group)
        {
            auto& [transform, mesh] = group.get<TransformComponent, SpriteRendererComponent>(entity);

        }

        auto view = m_registry.view<TransformComponent>();
        for (auto entity : view)
        {

        }
        */
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
    void Scene::OnUpdate(Timestep timestep)
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
        glm::mat4* cameraTransform = nullptr;
        {
            auto group = m_registry.group<TransformComponent, CameraComponent>();
            for (auto entity : group)
            {
                auto [transform, camera] = group.get<TransformComponent, CameraComponent>(entity);

                if (camera.Primary)
                {
                    mainCamera = &camera.Camera;
                    cameraTransform = &transform.Transform;
                    break;
                }
            }
        }

        if(mainCamera)
        {   
            Renderer2D::BeginScene(mainCamera->GetProjection(), *cameraTransform);
            auto group = m_registry.group<SpriteRendererComponent>(entt::get<TransformComponent>);

            for (auto entity : group)
            {
                auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);
                

                Renderer2D::DrawQuad(transform.Transform, sprite.Color);
            }

            Renderer2D::EndScene();
        }


    }

    void Scene::OnViewportResize(uint32_t width, uint32_t height)
    {
        m_viewportHeight = height;
        m_viewportWidth = width;

        auto view = m_registry.view<CameraComponent>();

        for (auto entity : view)
        {
            auto cameraComp = view.get<CameraComponent>(entity);
            if (cameraComp.FixedAspectRatio)
            {
                cameraComp.Camera.SetViewportSize(width, height);
            }

        }

    }

  
}
