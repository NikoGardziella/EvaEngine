#include "EditorDebugUtils.h"
#include "Engine/Scene/Component.h "
#include "Engine/Core/Log.h"

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
}
