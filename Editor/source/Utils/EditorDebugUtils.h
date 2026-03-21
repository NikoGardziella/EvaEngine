#pragma once
#include "Engine/Core/core.h"
#include "glm/glm.hpp"
#include "Engine/Renderer/Renderer2D/VulkanRenderer2D.h"
#include <Engine/Scene/Components/Map/AreaComponent.h>

namespace Engine {

    class Scene;
    class EditorUtils
    {

    public:
        static void EditorUtils::DrawAreaDebugBounds(Ref<Scene> scene)
        {

            scene->ForEach<AreaComponent>(
                [](Engine::Entity e, AreaComponent& area)
                {
                    // 1. Calculate the center of the AABB
                    glm::vec2 center = (area.Min + area.Max) * 0.5f;

                    // 2. Calculate the size (width and height)
                    glm::vec2 size = area.Max - area.Min;

                    // 3. Add a small Z-offset so the lines sit slightly above the ground
                    glm::vec3 translation = glm::vec3(center, 0.05f);

                    // 4. Scale represents the full dimensions of the box
                    // Note: DrawLineRect usually expects the full width/height in scale
                    glm::vec3 scale = glm::vec3(size, 1.0f);

                    // 5. Build the transform matrix (No rotation needed for AABB)
                    glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation)
                        * glm::scale(glm::mat4(1.0f), scale);

                    // 6. Draw with a distinct color (e.g., Cyan for Areas)
                    Engine::VulkanRenderer2D::DrawLineRect(transform, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f), -1);
                });
        }
    };

   
}
