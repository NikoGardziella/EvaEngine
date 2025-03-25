#pragma once
#include "Engine/Scene/Scene.h"

namespace Engine {

    class DebugUtils {
    public:
        static void LogAllEntitiesWithTags(const Ref<Scene>& scene);
        static void LogAllEntitiesWithComponents(const Ref<Scene>& scene);


        template<typename T>
        static void LogAllEntitiesWithComponent(const Ref<Scene>& scene, const std::string& componentName);
    };
}

