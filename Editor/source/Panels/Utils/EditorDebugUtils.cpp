#include "EditorDebugUtils.h"
#include "Engine/Scene/Component.h "
#include "Engine/Core/Log.h"
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Components/Map/AreaComponent.h>
#include <Engine/Renderer/Renderer2D/VulkanRenderer2D.h>

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
}
