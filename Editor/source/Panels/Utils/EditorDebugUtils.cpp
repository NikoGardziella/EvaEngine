#include "EditorDebugUtils.h"
#include "Engine/Scene/Component.h "
#include "Engine/Core/Log.h"
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Components/Map/AreaComponent.h>
#include <Engine/Renderer/Renderer2D/VulkanRenderer2D.h>
#include "Engine/Map/Utils/IsoTileUtils.h"

namespace Engine {

    void EditorDebugUtils::PrintAllEntities(Scene* scene)
    {
        EE_CORE_INFO("---- Dumping all entities ----");

		uint32_t entityCount = 0;

        auto group = scene->GetRegistry().group<TagComponent>();
        for (auto entity : group)
        {
            auto [tag] = group.get<TagComponent>(entity);
			EE_CORE_INFO("Entity Tag: {}",  tag);
			entityCount++;
        }
        EE_CORE_INFO("---- Total entities {}. End dump ----", entityCount);
    }


    void EditorDebugUtils::DrawAreaDebugBounds(Ref<Scene> scene)
    {

        scene->ForEachConst<AreaComponent>(
            [](Engine::Entity e, const AreaComponent& area)
            {
                glm::vec2 center = (area.Min + area.Max) * 0.5f;

                glm::vec2 size = area.Max - area.Min;

                glm::vec3 translation = glm::vec3(center, 0.05f);

                glm::vec3 scale = glm::vec3(size, 1.0f);

                glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation)
                    * glm::scale(glm::mat4(1.0f), scale);

                

                Engine::VulkanRenderer2D::DrawLineRect(transform, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f), -1);
            });
    }

    void EditorDebugUtils::DrawWorldAxes(const glm::vec2& cameraWorldPos, float extent)
    {
        // Horizontal axis (Y = 0)
        {
            glm::vec3 p0 = { cameraWorldPos.x - extent, 0.0f, 0.0f };
            glm::vec3 p1 = { cameraWorldPos.x + extent, 0.0f, 0.0f };

            VulkanRenderer2D::DrawLine(p0, p1, glm::vec4(1.0f, 0.2f, 0.2f, 1.0f));
        }

        // Vertical axis (X = 0)
        {
            glm::vec3 p0 = { 0.0f, cameraWorldPos.y - extent, 0.0f };
            glm::vec3 p1 = { 0.0f, cameraWorldPos.y + extent, 0.0f };

            VulkanRenderer2D::DrawLine(p0, p1, glm::vec4(0.2f, 1.0f, 0.2f, 1.0f));
        }
    }


    void EditorDebugUtils::DrawIsometricGrid(const glm::vec2& cameraWorldPos, float extent)
    {
        glm::ivec2 centerCell = IsoTileUtils::WorldToIsoCell(cameraWorldPos);

        auto GridToWorld = [&](int gx, int gy) -> glm::vec3
            {
                glm::vec2 p = IsoTileUtils::IsoToWorldGround({ gx, gy });
                glm::vec2 gridOffset = { 0.5f, 0.35f };
                p += gridOffset;
                return { p.x, p.y, 0.0f };
            };

        int minGX = centerCell.x - static_cast<int>(extent);
        int maxGX = centerCell.x + static_cast<int>(extent);
        int minGY = centerCell.y - static_cast<int>(extent);
        int maxGY = centerCell.y + static_cast<int>(extent);

        for (int gx = minGX; gx <= maxGX; ++gx)
        {
            glm::vec3 p0 = GridToWorld(gx, minGY);
            glm::vec3 p1 = GridToWorld(gx, maxGY);

            glm::vec4 color = glm::vec4(0.35f, 0.35f, 0.35f, 0.1f);

            VulkanRenderer2D::DrawLineUnderlay(p0, p1, color);
        }

        for (int gy = minGY; gy <= maxGY; ++gy)
        {
            glm::vec3 p0 = GridToWorld(minGX, gy);
            glm::vec3 p1 = GridToWorld(maxGX, gy);

            glm::vec4 color =  glm::vec4(0.35f, 0.35f, 0.35f, 0.1f);

            VulkanRenderer2D::DrawLineUnderlay(p0, p1, color);
        }
    }
};



