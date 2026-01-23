#include "pch.h"

#include "Components/Player/CharacterControllerComponent.h"
#include "Components/Render/TileComponent.h"

#include "Engine/Renderer/Renderer2D/VulkanRenderer2D.h"

namespace Engine {



    void Scene::OnUpdateEditor(Timestep timestep, EditorCamera& camera)
    {
        EE_PROFILE_FUNCTION();

        Engine::VulkanRenderer2D::BeginScene(camera);


        {

            auto playerView = m_registry.view<Engine::TransformComponent, CharacterControllerComponent, Engine::CircleCollider2DComponent, Engine::IDComponent>();
            glm::vec2 playerPos;

            for (auto playerEntity : playerView)
            {
                auto& playerTransform = playerView.get<Engine::TransformComponent>(playerEntity);
                playerPos.x = playerTransform.Translation.x;
                playerPos.y = playerTransform.Translation.y;

            }

            // m_textureStreamingSystem->Update(playerPos, this);
        }


        {
            auto view = m_registry.view<SpriteRendererComponent, TransformComponent>();

            for (auto entity : view)
            {
                auto [transform, sprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);

                //Engine::VulkanRenderer2D::DrawQuad(transform.GetTransform(), sprite.Color);

               // Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);

            }
        }

        {
            auto view = m_registry.view<CircleRendererComponent, TransformComponent>();

            for (auto entity : view)
            {
                auto [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);

                // Renderer2D::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness, circle.Fade, (int)entity);

            }
        }


        {
            auto view = m_registry.view<SpriteRendererComponent, TransformComponent>();
            for (auto entity : view)
            {
                auto [transform, quadSprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);
                float tiling = 1.0f;
                if (quadSprite.Texture == nullptr)
                {
                    continue;
                }

                //Engine::VulkanRenderer2D::DrawTextureQuad(transform.GetTransform(), quadSprite.Texture, tiling, quadSprite.Color);
            }
        }
        {
            auto view = m_registry.view<TileComponent, TransformComponent>();
            for (auto entity : view)
            {
                auto [transform, quadSprite] = view.get<TransformComponent, TileComponent>(entity);
                float tiling = 1.0f;
                if (quadSprite.Texture == nullptr)
                {
                    continue;
                }
                glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                //Engine::VulkanRenderer2D::DrawTextureQuad(transform.GetTransform(), quadSprite.Texture, tiling, color);
            }
        }

        {

            auto viewTerrain = m_registry.view<TileComponent, TransformComponent>();
            const float step = float(TILE_SIZE);
            viewTerrain.use<TransformComponent>();

            for (auto entity : viewTerrain)
            {
                auto& tileComp = viewTerrain.get<TileComponent>(entity);
                auto& tr = viewTerrain.get<TransformComponent>(entity);

                for (const auto& t : tileComp.tiles)
                {
                    if (t.Category != eTileCategory::Terrain) continue;

                    // t.position is a WORLD delta (not iso). Do NOT round/convert it.
                    glm::vec2 worldPosCenter = glm::vec2(tr.Translation) + t.position;

                    // bottom tip (ground contact) for bottom-center pivot

                    // Flip V like before
                    glm::vec4 uv = t.UV;
                    glm::vec4 flippedUV(uv.x, uv.w, uv.z, uv.y);

                    Engine::VulkanRenderer2D::DrawTile(worldPosCenter, flippedUV, glm::vec4(1.0f));
                }
            }





            Engine::VulkanRenderer2D::EndScene();

            Engine::VulkanRenderer2D::BeginScene(camera);

            auto view = m_registry.view<TileComponent, TransformComponent>();
            view.use<TransformComponent>(); // this ensured the draw order!
            for (auto entity : view)
            {

                TileComponent& tileComponent = view.get<TileComponent>(entity);
                TransformComponent& transformComponent = view.get<TransformComponent>(entity);

                const float step = float(TILE_SIZE);
                for (size_t i = 0; i < tileComponent.tiles.size(); i++)
                {
                    if (tileComponent.tiles[i].Category == eTileCategory::Terrain)
                    {
                        // skip terrain and draw everything else 
                        continue;
                    }
                    glm::vec2 ground = glm::vec2(transformComponent.Translation) + tileComponent.tiles[i].position; // position = WORLD delta to GROUND

                    // Flip V as before
                    glm::vec4 uv = tileComponent.tiles[i].UV;
                    glm::vec4 flippedUV(uv.x, uv.w, uv.z, uv.y);

                    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                    // Use flippedUV for rendering, don't overwrite original UV
                    Engine::VulkanRenderer2D::DrawTile(ground, flippedUV, color);
                }

            }

        }
        //Engine::Renderer::DrawFrame();
        Engine::VulkanRenderer2D::EndScene();
    }
}