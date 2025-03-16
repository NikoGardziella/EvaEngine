#pragma once

#include "Engine/Core/Core.h"
#include <Engine/Core/UUID.h>
#include "Engine/Renderer/Texture.h"
#include "SceneCamera.h"

#include <box2d/id.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>



namespace Engine {


    struct IDComponent
    {
        UUID ID;

        IDComponent() = default;
        IDComponent(const IDComponent&) = default;

    };

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
        Ref<Texture2D> Texture;
        float Tiling = 1.0f;
        SpriteRendererComponent() = default;
        SpriteRendererComponent(const SpriteRendererComponent&) = default;
        SpriteRendererComponent(const glm::vec4& color)
            : Color(color) { }

        
    };

    struct CircleRendererComponent
    {
        glm::vec4 Color{ 1.0f };
        float Thickness = 1.0f;
        float Fade = 0.001f;

        CircleRendererComponent() = default;
        CircleRendererComponent(const CircleRendererComponent&) = default;
       


    };

    // forward declaration because ScriptableEntity has Entity.h
    class ScriptableEntity;
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


    // Physics

    struct RigidBody2DComponent {

        enum class BodyType {
            Static = 0,
            Dynamic = 1,
            Kinematic = 2
        };

        BodyType Type = BodyType::Static;
        bool FixedRotation = false;

        //Storage for runtiem
        b2BodyId RuntimeBody;

        RigidBody2DComponent() = default;
        RigidBody2DComponent(const RigidBody2DComponent&) = default;

    };

    struct BoxCollider2DComponent {

        glm::vec2 Offset = { 0.0f, 0.0f, };
        glm::vec2 Size = { 0.5f, 0.5f, }; // for 1x1 object

        // Move to physics material?
        float Density = 1.0f;
        float Friction = 0.5f;
        float Restitution = 0.0f;
        float RestitutionThershold = 0.5f; // stop physic when below this value

        //Storage for runtiem
        void* RuntimeFixture = nullptr;
        b2ShapeId shapeID;

        BoxCollider2DComponent() = default;
        BoxCollider2DComponent(const BoxCollider2DComponent&) = default;

    };

    struct CircleCollider2DComponent {

        glm::vec2 Offset = { 0.0f, 0.0f, };
        float Radius = 0.5f;
        // Move to physics material?
        float Density = 1.0f;
        float Friction = 0.5f;
        float Restitution = 0.0f;
        float RestitutionThershold = 0.5f; // stop physic when below this value

        //Storage for runtiem
        void* RuntimeFixture = nullptr;
        b2ShapeId shapeID;

        CircleCollider2DComponent() = default;
        CircleCollider2DComponent(const CircleCollider2DComponent&) = default;
        
    };
}