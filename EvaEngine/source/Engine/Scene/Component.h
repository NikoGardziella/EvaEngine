#pragma once
#include "Engine/Core/Core.h"
#include <glm/glm.hpp>
#include "SceneCamera.h"
#include <string>
#include "ScriptableEntity.h"


namespace Engine {

    struct CameraComponent
    {
        SceneCamera Camera;
        bool Primary = true;
        bool FixedAspectRatio = false;

        CameraComponent() = default;
        CameraComponent(const CameraComponent&) = default;

    };


    struct TagComponent
    {
        std::string Tag;
        TagComponent(const TagComponent&) = default;
        TagComponent(const std::string tag)
            : Tag(tag) { }

    };

    struct TransformComponent
    {
        glm::mat4 Transform{ 1.0f };
        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const glm::mat4 transform)
            : Transform(transform) {
        }

        operator glm::mat4& () { return Transform; }
        operator const glm::mat4& () const { return Transform; }
    };

    struct SpriteRendererComponent
    {
        glm::vec4 Color{ 1.0f };
        SpriteRendererComponent() = default;
        SpriteRendererComponent(const SpriteRendererComponent&) = default;
        SpriteRendererComponent(const glm::vec4& color)
            : Color(color) { }

        
    };


    struct NativeScriptComponent
    {

        // change this to something better. Use ECS

        ScriptableEntity* Instance = nullptr;

        FUNCTION_POINTER(InstantiateScript);
        void (*DestroyScript)(ScriptableEntity*);

        template<typename T>
        void Bind()
        {


            InstantiateScript = []() -> ScriptableEntity*
                {
                    return new T();
                };

            DestroyScript = [](ScriptableEntity* instance)
                {
                   delete instance;
                };
        }
    };



}