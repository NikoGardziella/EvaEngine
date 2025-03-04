#pragma once
#include "Engine/Core/Core.h"
#include <glm/glm.hpp>
#include "SceneCamera.h"
#include <string>
#include "ScriptableEntity.h"
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>


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
        // should this be name?!
        std::string Tag;
        TagComponent(const TagComponent&) = default;
        TagComponent(const std::string tag)
            : Tag(tag) { }

    };

    struct TransformComponent
    {
        glm::vec3 Translation { 0.0f, 0.0f, 0.0f };
        glm::vec3 Rotation { 0.0f, 0.0f, 0.0f };
        glm::vec3 Scale { 1.0f, 1.0f, 1.0f };


        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const glm::vec3 translation)
            : Translation(translation) {
        }

        glm::mat4 GetTransform() const
        {
            glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));

            return glm::translate(glm::mat4(1.0f), Translation)
                * rotation
                * glm::scale(glm::mat4(1.0f), Scale);
        }
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