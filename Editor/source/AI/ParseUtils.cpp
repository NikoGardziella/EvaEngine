#include "ParseUtils.h"
#include <Engine/AssetManager/AssetManager.h>
#include <Engine/Scene/Components/Combat/HealthComponent.h>
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>

template<typename T>
bool ParseUtils::SafeGet(const nlohmann::json& j, const std::string& key, T& out)
{
    if (j.contains(key) && j[key].is_null() == false && j[key].is_primitive())
    {
        try
        {
            out = j[key].get<T>();
            return true;
        }
        catch (...)
        {
            return false;
        }
    }
    return false;
}

bool ParseUtils::SafeGetVec2(const nlohmann::json& j, const std::string& key, glm::vec2& out)
{
    if (j.contains(key) && j[key].is_array() && j[key].size() >= 2)
    {
        try
        {
            out = glm::vec2(j[key][0].get<float>(), j[key][1].get<float>());
            return true;
        }
        catch (...)
        {
            return false;
        }
    }
    return false;
}

bool ParseUtils::SafeGetVec3(const nlohmann::json& j, const std::string& key, glm::vec3& out)
{
    if (j.contains(key) && j[key].is_array() && j[key].size() >= 3)
    {
        try
        {
            out = glm::vec3(j[key][0].get<float>(), j[key][1].get<float>(), j[key][2].get<float>());
            return true;
        }
        catch (...) { return false; }
    }
    return false;
}


bool ParseUtils::ParseComponent(std::string compName, Engine::Entity entity, const nlohmann::json& compData)
{

    if (compName == "TransformComponent")
    {
        Engine::TransformComponent& tc = entity.HasComponent<Engine::TransformComponent>()
            ? entity.GetComponent< Engine::TransformComponent>()
            : entity.AddComponent< Engine::TransformComponent>();


        SafeGetVec3(compData, "Translation", tc.Translation);
        SafeGetVec3(compData, "Rotation", tc.Rotation);
        SafeGetVec3(compData, "Scale", tc.Scale);
        /*
        tc.Translation = glm::vec3(
            compData["Translation"][0],
            compData["Translation"][1],
            compData["Translation"][2]
        );
        tc.Rotation = glm::vec3(
            compData["Rotation"][0],
            compData["Rotation"][1],
            compData["Rotation"][2]
        );
        tc.Scale = glm::vec3(
            compData["Scale"][0],
            compData["Scale"][1],
            compData["Scale"][2]
        );
        */
    }
    else if (compName == "SpriteRendererComponent")
    {
        Engine::SpriteRendererComponent& src = entity.HasComponent<Engine::SpriteRendererComponent>()
            ? entity.GetComponent<Engine::SpriteRendererComponent>()
            : entity.AddComponent<Engine::SpriteRendererComponent>();



        /*
         src.Color = glm::vec4(
             compData["Color"][0].get<float>(),
             compData["Color"][1].get<float>(),
             compData["Color"][2].get<float>(),
             compData["Color"][3].get<float>()
         );

        */
        if (compData.contains("Texture") && !compData["Texture"].is_null())
        {
            std::string textureName = compData["Texture"];
            src.Texture = Engine::AssetManager::GetTexture(textureName);; // You must implement LoadTexture
        }

        if (compData.contains("Tiling"))
        {
            src.Tiling = compData["Tiling"];
        }

    }
    else if (compName == "CameraComponent")
    {
        Engine::CameraComponent& cc = entity.AddComponent<Engine::CameraComponent>();

        cc.Primary = compData["Primary"];
        cc.FixedAspectRatio = compData["FixedAspectRatio"];
    }
 
    else if (compName == "CharacterControllerComponent")
    {
        CharacterControllerComponent& cc = entity.AddComponent<CharacterControllerComponent>();

        cc.speed = compData["speed"];
        cc.velocity = glm::vec2(compData["velocity"][0], compData["velocity"][1]);
        cc.onGround = compData["onGround"];
        cc.fireRate = compData["fireRate"];
        cc.lastFireTime = compData["lastFireTime"];
    }
    else if (compName == "ProjectileComponent")
    {
        Engine::ProjectileComponent& pc = entity.AddComponent<Engine::ProjectileComponent>();

        pc.Velocity = glm::vec2(compData["Velocity"][0], compData["Velocity"][1]);
        pc.LifeTime = compData["LifeTime"];
    }
    else if (compName == "RigidBody2DComponent")
    {
        Engine::RigidBody2DComponent& rb = entity.AddComponent<Engine::RigidBody2DComponent>();

        rb.Type = static_cast<Engine::RigidBody2DComponent::BodyType>(compData["Type"]);
        rb.FixedRotation = compData["FixedRotation"];

    }
    else if (compName == "BoxCollider2DComponent")
    {
        Engine::BoxCollider2DComponent& bc = entity.HasComponent<Engine::BoxCollider2DComponent>()
            ? entity.GetComponent<Engine::BoxCollider2DComponent>()
            : entity.AddComponent<Engine::BoxCollider2DComponent>();


        SafeGetVec2(compData, "Offset", bc.Offset);
        SafeGetVec2(compData, "Size", bc.Size);
        SafeGet(compData, "Density", bc.Density);
        SafeGet(compData, "Friction", bc.Friction);
        SafeGet(compData, "Restitution", bc.Restitution);
        SafeGet(compData, "RestitutionThershold", bc.RestitutionThershold);

    }
    else if (compName == "CircleCollider2DComponent")
    {
        Engine::CircleCollider2DComponent& cc = entity.AddComponent<Engine::CircleCollider2DComponent>();

        cc.Offset = glm::vec2(compData["Offset"][0], compData["Offset"][1]);
        cc.Radius = compData["Radius"];
        cc.Density = compData["Density"];
        cc.Friction = compData["Friction"];
        cc.Restitution = compData["Restitution"];
        cc.RestitutionThershold = compData["RestitutionThershold"];
    }
    else if (compName == "HealthComponent")
    {
        Engine::HealthComponent& healthComp = entity.HasComponent<Engine::HealthComponent>()
            ? entity.GetComponent<Engine::HealthComponent>()
            : entity.AddComponent<Engine::HealthComponent>();

		//healthComp.Current = compData["Health"];
        SafeGet(compData, "Health", healthComp.Max);
    }
    return true;
}
