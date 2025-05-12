#pragma once

#include "Engine/Core/Core.h"
#include <Engine/Core/UUID.h>
#include "Engine/Renderer/Texture.h"
#include "Engine/Platform/Vulkan/Pixel/VulkanPixelTexture.h"
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

    struct ProjectileComponent {
        glm::vec2 Velocity;   // units per second
        float      LifeTime;   // seconds remaining
        float      Damage = 10.0f;

        ProjectileComponent() = default;
        ProjectileComponent(const glm::vec2& velocity, float lifeTime)
            : Velocity(velocity), LifeTime(lifeTime) {
        }
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

        void SetTransform(const glm::mat4& transform)
        {
            Translation = glm::vec3(transform[3]);  

            Scale.x = glm::length(glm::vec3(transform[0]));
            Scale.y = glm::length(glm::vec3(transform[1]));
            Scale.z = glm::length(glm::vec3(transform[2]));

            glm::mat3 rotationMatrix = glm::mat3(
                glm::vec3(transform[0]) / Scale.x,
                glm::vec3(transform[1]) / Scale.y,
                glm::vec3(transform[2]) / Scale.z
            );

            Rotation = glm::eulerAngles(glm::quat_cast(rotationMatrix)); // Convert to Euler angles
        }
    };

    struct SpriteRendererComponent
    {
        glm::vec4 Color{ 1.0f };
        Ref<VulkanTexture> Texture;
        float Tiling = 1.0f;
        SpriteRendererComponent() = default;
        SpriteRendererComponent(const SpriteRendererComponent&) = default;
        SpriteRendererComponent(const glm::vec4& color)
            : Color(color) { }

        
    };

    struct PixelSpriteRendererComponent
    {
        glm::vec4 Color{ 1.0f };
        Ref<VulkanPixelTexture> Texture;
        float Tiling = 1.0f;
        PixelSpriteRendererComponent() = default;
        PixelSpriteRendererComponent(const PixelSpriteRendererComponent&) = default;
        PixelSpriteRendererComponent(const glm::vec4& color)
            : Color(color) {
        }


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

    struct CharacterControllerComponent
    {
        float speed = 5.0f;
        glm::vec2 velocity = { 0.0f, 0.0f };
        bool onGround = false;
		float fireRate = 0.5f;
		float lastFireTime = 0.0f;


        CharacterControllerComponent() = default;
        CharacterControllerComponent(const CharacterControllerComponent&) = default;
    };

}