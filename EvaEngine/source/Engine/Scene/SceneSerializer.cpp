#include "pch.h"
#include "SceneSerializer.h"
#include "Entity.h"     
#include "Component.h" 

#include "Components/Combat/HealthComponent.h"
#include "Components/NPC/NpcAIComponent.h"
#include "Engine/AssetManager/AssetManager.h"

#include <filesystem>
#include <fstream>
#include <yaml-cpp/yaml.h>
#include "Components/Player/CharacterControllerComponent.h"
#include "Components/Combat/WeaponComponent.h"


namespace Engine {

	SceneSerializer::SceneSerializer(const Ref<Scene> scene)
		: m_scene(scene)
	{

	}


    // Helper functions for serializing individual components
    namespace SerializeUtils
    {
        inline void SerializeTagComponent(Entity entity, YAML::Emitter& out)
        {
            auto& tag = entity.GetComponent<TagComponent>();
            out << YAML::Key << "TagComponent" << YAML::BeginMap;
            out << YAML::Key << "Tag" << YAML::Value << tag.Tag;
            out << YAML::EndMap;
        }

        inline void SerializeTransformComponent(Entity entity, YAML::Emitter& out)
        {
            auto& transform = entity.GetComponent<TransformComponent>();
            out << YAML::Key << "TransformComponent" << YAML::Value;
            out << YAML::BeginMap;
            out << YAML::Key << "Translation" << YAML::Value << YAML::Flow
                << std::vector<float>{ transform.Translation.x, transform.Translation.y, transform.Translation.z };
            out << YAML::Key << "Rotation" << YAML::Value << YAML::Flow
                << std::vector<float>{ transform.Rotation.x, transform.Rotation.y, transform.Rotation.z };
            out << YAML::Key << "Scale" << YAML::Value << YAML::Flow
                << std::vector<float>{ transform.Scale.x, transform.Scale.y, transform.Scale.z };
            out << YAML::EndMap;
        }

        inline void SerializeCameraComponent(Entity entity, YAML::Emitter& out)
        {
            auto& cameraComponent = entity.GetComponent<CameraComponent>();

            out << YAML::Key << "CameraComponent" << YAML::Value;
            out << YAML::BeginMap;

            out << YAML::Key << "Primary" << YAML::Value << cameraComponent.Primary;
            out << YAML::Key << "FixedAspectRatio" << YAML::Value << cameraComponent.FixedAspectRatio;

            // Optionally serialize additional SceneCamera data
            out << YAML::Key << "ProjectionType" << YAML::Value;
            if (cameraComponent.Camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
                out << "Perspective";
            else
                out << "Orthographic";

            if (cameraComponent.Camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
            {
                out << YAML::Key << "PerspectiveFOV" << YAML::Value << cameraComponent.Camera.GetPerspectiveFOV();
                out << YAML::Key << "PerspectiveNearClip" << YAML::Value << cameraComponent.Camera.GetPerspectiveNearClip();
                out << YAML::Key << "PerspectiveFarClip" << YAML::Value << cameraComponent.Camera.GetPerspectiveFarClip();
            }
            else
            {
                out << YAML::Key << "OrthographicSize" << YAML::Value << cameraComponent.Camera.GetOrthographicSize();
                out << YAML::Key << "OrthographicNearClip" << YAML::Value << cameraComponent.Camera.GetOrthographicNearClip();
                out << YAML::Key << "OrthographicFarClip" << YAML::Value << cameraComponent.Camera.GetOrthographicFarClip();
            }

            out << YAML::EndMap;
        }


        inline void SerializeSpriteRendererComponent(Entity entity, YAML::Emitter& out)
        {
            auto& sprite = entity.GetComponent<SpriteRendererComponent>();
            out << YAML::Key << "SpriteRendererComponent" << YAML::Value;
            out << YAML::BeginMap;

            out << YAML::Key << "Color" << YAML::Value << YAML::Flow
                << std::vector<float>{ sprite.Color.r, sprite.Color.g, sprite.Color.b, sprite.Color.a };

            if (sprite.Texture)
                out << YAML::Key << "Texture" << YAML::Value << sprite.Texture->GetName();
            else
                out << YAML::Key << "Texture" << YAML::Value << "";

            
            out << YAML::Key << "Tiling" << YAML::Value << sprite.Tiling;

            out << YAML::EndMap;
        }


        inline void SerializeNativeScriptComponent(Entity entity, YAML::Emitter& out)
        {
            // For demonstration, we simply output that a native script is attached.
            out << YAML::Key << "NativeScriptComponent" << YAML::Value << "Script Attached";
        }

        inline void SerializeRigidBody2DComponent(Entity entity, YAML::Emitter& out)
        {
            auto& rigidBodyComp = entity.GetComponent<RigidBody2DComponent>();

            out << YAML::Key << "RigidBody2DComponent";
            out << YAML::BeginMap;

            out << YAML::Key << "Type" << YAML::Value << static_cast<int>(rigidBodyComp.Type);
            out << YAML::Key << "FixedRotation" << YAML::Value << rigidBodyComp.FixedRotation;

            out << YAML::EndMap;
        }

        inline void SerializeBoxCollider2DComponent(Entity entity, YAML::Emitter& out)
        {
            auto& colliderComp = entity.GetComponent<BoxCollider2DComponent>();

            out << YAML::Key << "BoxCollider2DComponent";
            out << YAML::BeginMap;

            out << YAML::Key << "Offset" << YAML::Value << YAML::Flow << std::vector<float>{colliderComp.Offset.x, colliderComp.Offset.y};
            out << YAML::Key << "Size" << YAML::Value << YAML::Flow << std::vector<float>{colliderComp.Size.x, colliderComp.Size.y};
            out << YAML::Key << "Density" << YAML::Value << colliderComp.Density;
            out << YAML::Key << "Friction" << YAML::Value << colliderComp.Friction;
            out << YAML::Key << "Restitution" << YAML::Value << colliderComp.Restitution;
            out << YAML::Key << "RestitutionThreshold" << YAML::Value << colliderComp.RestitutionThershold;

            out << YAML::EndMap;
        }
        inline void SerializeCircleRendererComponent(Entity entity, YAML::Emitter& out)
        {
            auto& circleComp = entity.GetComponent<CircleRendererComponent>();

            out << YAML::Key << "CircleRendererComponent";
            out << YAML::BeginMap;

            out << YAML::Key << "Color" << YAML::Value << YAML::Flow
                << std::vector<float>{circleComp.Color.r, circleComp.Color.g, circleComp.Color.b, circleComp.Color.a};

            out << YAML::Key << "Thickness" << YAML::Value << circleComp.Thickness;
            out << YAML::Key << "Fade" << YAML::Value << circleComp.Fade;

            out << YAML::EndMap;
        }

        inline void SerializeCircleCollider2DComponent(Entity entity, YAML::Emitter& out)
        {
            auto& colliderComp = entity.GetComponent<CircleCollider2DComponent>();

            out << YAML::Key << "CircleCollider2DComponent";
            out << YAML::BeginMap;

            out << YAML::Key << "Offset" << YAML::Value << YAML::Flow << std::vector<float>{colliderComp.Offset.x, colliderComp.Offset.y};
            out << YAML::Key << "Radius" << YAML::Value << YAML::Flow << colliderComp.Radius;
            out << YAML::Key << "Density" << YAML::Value << colliderComp.Density;
            out << YAML::Key << "Friction" << YAML::Value << colliderComp.Friction;
            out << YAML::Key << "Restitution" << YAML::Value << colliderComp.Restitution;
            out << YAML::Key << "RestitutionThreshold" << YAML::Value << colliderComp.RestitutionThershold;

            out << YAML::EndMap;
        }

        inline void SerializeHealthComponent(Entity entity, YAML::Emitter& out)
        {
            HealthComponent& healthComp = entity.GetComponent<HealthComponent>();

            out << YAML::Key << "HealthComponent";
            out << YAML::BeginMap;


            out << YAML::Key << "Current" << YAML::Value << healthComp.Current;
            out << YAML::Key << "Max" << YAML::Value << healthComp.Max;

            out << YAML::EndMap;
        }
        inline void SerializeNPCAIMovementComponent(Entity entity, YAML::Emitter& out)
        {
            const NPCAIMovementComponent& comp = entity.GetComponent<NPCAIMovementComponent>();

            out << YAML::Key << "NPCAIMovementComponent";
            out << YAML::BeginMap;

            out << YAML::Key << "CurrentState" << YAML::Value << static_cast<int>(comp.CurrentState);

            out << YAML::Key << "PatrolPoints" << YAML::Value << YAML::BeginSeq;
            for (const auto& point : comp.PatrolPoints)
            {
                out << YAML::Flow << YAML::BeginSeq << point.x << point.y << point.z << YAML::EndSeq;
            }
            out << YAML::EndSeq;

            out << YAML::Key << "CurrentPatrolIndex" << YAML::Value << comp.CurrentPatrolIndex;
            out << YAML::Key << "IdleDuration" << YAML::Value << comp.IdleDuration;
            out << YAML::Key << "IdleTimer" << YAML::Value << comp.IdleTimer;

            out << YAML::Key << "TargetPosition" << YAML::Value
                << YAML::Flow << YAML::BeginSeq << comp.TargetPosition.x << comp.TargetPosition.y << comp.TargetPosition.z << YAML::EndSeq;

            out << YAML::Key << "MoveSpeed" << YAML::Value << comp.MoveSpeed;

            out << YAML::EndMap;
        }

        inline void SerializeNPCAIVisionComponent(Entity entity, YAML::Emitter& out)
        {
            const NPCAIVisionComponent& comp = entity.GetComponent<NPCAIVisionComponent>();

            out << YAML::Key << "NPCAIVisionComponent";
            out << YAML::BeginMap;

            out << YAML::Key << "ViewRadius" << YAML::Value << comp.ViewRadius;
            out << YAML::Key << "ViewAngle" << YAML::Value << comp.ViewAngle;
            out << YAML::Key << "HasLineOfSight" << YAML::Value << comp.HasLineOfSight;

            out << YAML::EndMap;
        }

        inline void SerializeCharacterControllerComponent(Entity entity, YAML::Emitter& out)
        {
            if (!entity.HasComponent<CharacterControllerComponent>())
                return;

            const auto& comp = entity.GetComponent<CharacterControllerComponent>();

            out << YAML::Key << "CharacterControllerComponent" << YAML::Value;
            out << YAML::BeginMap;

            out << YAML::Key << "Speed" << YAML::Value << comp.speed;
            out << YAML::Key << "Velocity" << YAML::Value << YAML::Flow << YAML::BeginSeq << comp.velocity.x << comp.velocity.y << YAML::EndSeq;
            out << YAML::Key << "OnGround" << YAML::Value << comp.onGround;

            out << YAML::EndMap;
        }

        inline void SerializeWeaponComponent(Entity entity,YAML::Emitter& out)
        {
            if (!entity.HasComponent<WeaponComponent>())
                return;

            const WeaponComponent& comp = entity.GetComponent<WeaponComponent>();

            out << YAML::Key << "WeaponComponent";
            out << YAML::BeginMap;

            out << YAML::Key << "Damage" << YAML::Value << comp.Damage;
            out << YAML::Key << "FireRate" << YAML::Value << comp.FireRate;
            // Cooldown and IsFiring are runtime values, usually not serialized
            // out << YAML::Key << "Cooldown"  << YAML::Value << comp.Cooldown;
            // out << YAML::Key << "IsFiring"  << YAML::Value << comp.IsFiring;

            out << YAML::EndMap;
        }


        // Serializes an individual entity by checking for each component.
        void SerializeEntity(Entity entity, YAML::Emitter& out)
        {
            EE_CORE_ASSERT(entity.HasComponent<IDComponent>());


            out << YAML::BeginMap;

            out << YAML::Key << "ID" << YAML::Value << entity.GetUUID();

            if (entity.HasComponent<TagComponent>())
                SerializeTagComponent(entity, out);
            if (entity.HasComponent<TransformComponent>())
                SerializeTransformComponent(entity, out);
            if (entity.HasComponent<CameraComponent>())
                SerializeCameraComponent(entity, out);
            if (entity.HasComponent<SpriteRendererComponent>())
                SerializeSpriteRendererComponent(entity, out);
            if (entity.HasComponent<NativeScriptComponent>())
                SerializeNativeScriptComponent(entity, out);
            if (entity.HasComponent<RigidBody2DComponent>())
                SerializeRigidBody2DComponent(entity, out);
            if (entity.HasComponent<BoxCollider2DComponent>())
                SerializeBoxCollider2DComponent(entity, out);
            if (entity.HasComponent<CircleRendererComponent>())
                SerializeCircleRendererComponent(entity, out);
            if (entity.HasComponent<CircleCollider2DComponent>())
                SerializeCircleCollider2DComponent(entity, out);
            if (entity.HasComponent<HealthComponent>())
                SerializeHealthComponent(entity, out);
            if (entity.HasComponent<NPCAIMovementComponent>())
                SerializeNPCAIMovementComponent(entity, out);
            if (entity.HasComponent<NPCAIVisionComponent>())
                SerializeNPCAIVisionComponent(entity, out);
            if (entity.HasComponent<CharacterControllerComponent>())
                SerializeCharacterControllerComponent(entity, out);
            if (entity.HasComponent<WeaponComponent>())
                SerializeWeaponComponent(entity, out);




            out << YAML::EndMap;
        }


        //************************* Deserialize ****************************************

        inline void DeserializeWeaponComponent(Entity entity, const YAML::Node& entityNode)
        {
            if (entityNode["WeaponComponent"])
            {
                WeaponComponent& comp = entity.AddComponent<WeaponComponent>();
                const auto& node = entityNode["WeaponComponent"];

                if (node["Damage"])
                    comp.Damage = node["Damage"].as<float>();

                if (node["FireRate"])
                    comp.FireRate = node["FireRate"].as<float>();

                // Cooldown and IsFiring are runtime values; reset to default
                comp.Cooldown = 0.0f;
                comp.IsFiring = false;
            }
        }



        inline void DeserializeHealthComponent(Entity entity, const YAML::Node& entityNode)
        {
            if (entityNode["HealthComponent"])
            {
                HealthComponent& healthComp = entity.AddComponent<HealthComponent>();
                float maxHealth = entityNode["HealthComponent"]["Max"].as<float>();
                float currentHealth = entityNode["HealthComponent"]["Current"].as<float>();
                
                healthComp.Max = maxHealth;
                healthComp.Current = currentHealth;
               
            }
        }

        inline void DeserializeNPCAIMovementComponent(Entity entity, const YAML::Node& entityNode)
        {
            if (entityNode["NPCAIMovementComponent"])
            {
                NPCAIMovementComponent& comp = entity.AddComponent<NPCAIMovementComponent>();
                const auto& node = entityNode["NPCAIMovementComponent"];

                if (node["CurrentState"])
                    comp.CurrentState = static_cast<AIState>(node["CurrentState"].as<int>());

                if (node["PatrolPoints"])
                {
                    for (const auto& point : node["PatrolPoints"])
                    {
                        glm::vec3 patrolPoint;
                        patrolPoint.x = point[0].as<float>();
                        patrolPoint.y = point[1].as<float>();
                        patrolPoint.z = point[2].as<float>();
                        comp.PatrolPoints.push_back(patrolPoint);
                    }
                }

                if (node["CurrentPatrolIndex"])
                    comp.CurrentPatrolIndex = node["CurrentPatrolIndex"].as<int>();

                if (node["IdleDuration"])
                    comp.IdleDuration = node["IdleDuration"].as<float>();

                if (node["IdleTimer"])
                    comp.IdleTimer = node["IdleTimer"].as<float>();

                if (node["TargetPosition"])
                {
                    const auto& tp = node["TargetPosition"];
                    comp.TargetPosition = {
                        tp[0].as<float>(),
                        tp[1].as<float>(),
                        tp[2].as<float>()
                    };
                }

                if (node["MoveSpeed"])
                    comp.MoveSpeed = node["MoveSpeed"].as<float>();
            }
        }

        inline void DeserializeNPCAIVisionComponent(Entity entity, const YAML::Node& entityNode)
        {
            if (entityNode["NPCAIVisionComponent"])
            {
                NPCAIVisionComponent& comp = entity.AddComponent<NPCAIVisionComponent>();
                const auto& node = entityNode["NPCAIVisionComponent"];

                if (node["ViewRadius"])
                    comp.ViewRadius = node["ViewRadius"].as<float>();

                if (node["ViewAngle"])
                    comp.ViewAngle = node["ViewAngle"].as<float>();

                if (node["HasLineOfSight"])
                    comp.HasLineOfSight = node["HasLineOfSight"].as<bool>();

                // VisibleTarget is runtime/internal, not deserialized
                comp.VisibleTarget = entt::null;
            }
        }


        inline void DeserializeTagComponent(Entity entity, const YAML::Node& entityNode)
        {
            if (entityNode["TagComponent"])
            {
                const auto& tagNode = entityNode["TagComponent"]["Tag"];
                if (!tagNode) return; // Ensure the "Tag" key exists

                if (entity.HasComponent<TagComponent>())
                {
                    entity.GetComponent<TagComponent>().Tag = tagNode.as<std::string>();
                }
                else
                {
                    entity.AddComponent<TagComponent>(tagNode.as<std::string>());
                }
            }
        }

        inline void DeserializeTransformComponent(Entity entity, const YAML::Node& entityNode)
        {
            if (entityNode["TransformComponent"])
            {
                auto& transform = entity.AddComponent<TransformComponent>();
                auto translation = entityNode["TransformComponent"]["Translation"].as<std::vector<float>>();
                auto rotation = entityNode["TransformComponent"]["Rotation"].as<std::vector<float>>();
                auto scale = entityNode["TransformComponent"]["Scale"].as<std::vector<float>>();

                transform.Translation = { translation[0], translation[1], translation[2] };
                transform.Rotation = { rotation[0], rotation[1], rotation[2] };
                transform.Scale = { scale[0], scale[1], scale[2] };
            }
        }

        inline void DeserializeCameraComponent(Entity entity, const YAML::Node& entityNode)
        {
            if (entityNode["CameraComponent"])
            {
                auto& cameraComponent = entity.AddComponent<CameraComponent>();
                cameraComponent.Primary = entityNode["CameraComponent"]["Primary"].as<bool>();
                cameraComponent.FixedAspectRatio = entityNode["CameraComponent"]["FixedAspectRatio"].as<bool>();

                auto projectionTypeStr = entityNode["CameraComponent"]["ProjectionType"].as<std::string>();
                SceneCamera::ProjectionType projectionType =
                    (projectionTypeStr == "Perspective") ? SceneCamera::ProjectionType::Perspective : SceneCamera::ProjectionType::Orthographic;
                cameraComponent.Camera.SetProjectionType(projectionType);

                if (projectionType == SceneCamera::ProjectionType::Perspective)
                {
                    cameraComponent.Camera.SetPerspectiveFOV(entityNode["CameraComponent"]["PerspectiveFOV"].as<float>());
                    cameraComponent.Camera.SetPerspectiveNearClip(entityNode["CameraComponent"]["PerspectiveNearClip"].as<float>());
                    cameraComponent.Camera.SetPerspectiveFarClip(entityNode["CameraComponent"]["PerspectiveFarClip"].as<float>());
                }
                else
                {
                    cameraComponent.Camera.SetOrthographicSize(entityNode["CameraComponent"]["OrthographicSize"].as<float>());
                    cameraComponent.Camera.SetOrthographicNearClip(entityNode["CameraComponent"]["OrthographicNearClip"].as<float>());
                    cameraComponent.Camera.SetOrthographicFarClip(entityNode["CameraComponent"]["OrthographicFarClip"].as<float>());
                }
            }
        }

        inline void DeserializeSpriteRendererComponent(Entity entity, const YAML::Node& entityNode)
        {
            if (entityNode["SpriteRendererComponent"])
            {
                SpriteRendererComponent& sprite = entity.AddComponent<SpriteRendererComponent>();
                auto color = entityNode["SpriteRendererComponent"]["Color"].as<std::vector<float>>();
                sprite.Color = { color[0], color[1], color[2], color[3] };

				sprite.Texture = AssetManager::GetTexture(entityNode["SpriteRendererComponent"]["Texture"].as<std::string>());
            }
        }

        inline void DeserializeNativeScriptComponent(Entity entity, const YAML::Node& entityNode)
        {
            if (entityNode["NativeScriptComponent"])
            {
                entity.AddComponent<NativeScriptComponent>();
                // You may need additional logic to properly instantiate scripts.
            }
        }

        inline void DeserializeRigidBody2DComponent(Entity entity, const YAML::Node& entityNode)
        {
            if (entityNode["RigidBody2DComponent"])
            {
                auto& rb2d = entity.AddComponent<RigidBody2DComponent>();

                rb2d.Type = static_cast<RigidBody2DComponent::BodyType>(entityNode["RigidBody2DComponent"]["Type"].as<int>());
                rb2d.FixedRotation = entityNode["RigidBody2DComponent"]["FixedRotation"].as<bool>();
            }
        }

        inline void DeserializeBoxCollider2DComponent(Entity entity, const YAML::Node& entityNode)
        {
            if (entityNode["BoxCollider2DComponent"])
            {
                auto& boxCollider = entity.AddComponent<BoxCollider2DComponent>();

                auto offset = entityNode["BoxCollider2DComponent"]["Offset"].as<std::vector<float>>();
                auto size = entityNode["BoxCollider2DComponent"]["Size"].as<std::vector<float>>();

                boxCollider.Offset = { offset[0], offset[1] };
                boxCollider.Size = { size[0], size[1] };

                boxCollider.Density = entityNode["BoxCollider2DComponent"]["Density"].as<float>();
                boxCollider.Friction = entityNode["BoxCollider2DComponent"]["Friction"].as<float>();
                boxCollider.Restitution = entityNode["BoxCollider2DComponent"]["Restitution"].as<float>();
                boxCollider.RestitutionThershold = entityNode["BoxCollider2DComponent"]["RestitutionThreshold"].as<float>();
            }
        }

        inline void DeserializeCircleCollider2DComponent(Entity entity, const YAML::Node& entityNode)
        {
            if (entityNode["CircleCollider2DComponent"])
            {
                auto& circleCollider = entity.AddComponent<CircleCollider2DComponent>();

                auto offset = entityNode["CircleCollider2DComponent"]["Offset"].as<std::vector<float>>();

                circleCollider.Offset = { offset[0], offset[1] };

                circleCollider.Radius = entityNode["CircleCollider2DComponent"]["Radius"].as<float>();
                circleCollider.Density = entityNode["CircleCollider2DComponent"]["Density"].as<float>();
                circleCollider.Friction = entityNode["CircleCollider2DComponent"]["Friction"].as<float>();
                circleCollider.Restitution = entityNode["CircleCollider2DComponent"]["Restitution"].as<float>();
                circleCollider.RestitutionThershold = entityNode["CircleCollider2DComponent"]["RestitutionThreshold"].as<float>();
            }
        }

        inline void DeserializeCircleRendererComponent(Entity entity, const YAML::Node& node)
        {
            if (!node["CircleRendererComponent"])
                return;

            auto& circleComp = entity.AddComponent<CircleRendererComponent>();

            if (node["CircleRendererComponent"]["Color"])
            {
                auto color = node["CircleRendererComponent"]["Color"].as<std::vector<float>>();
                if (color.size() == 4)
                {
                    circleComp.Color = { color[0], color[1], color[2], color[3] };
                }
            }

            if (node["CircleRendererComponent"]["Thickness"])
                circleComp.Thickness = node["CircleRendererComponent"]["Thickness"].as<float>();

            if (node["CircleRendererComponent"]["Fade"])
                circleComp.Fade = node["CircleRendererComponent"]["Fade"].as<float>();
        }

        inline void DeserializeCharacterControllerComponent(Entity entity, const YAML::Node& entityNode)
        {
            if (entityNode["CharacterControllerComponent"])
            {
                CharacterControllerComponent& comp = entity.AddComponent<CharacterControllerComponent>();
                const auto& node = entityNode["CharacterControllerComponent"];

                if (node["Speed"])
                    comp.speed = node["Speed"].as<float>();

                if (node["Velocity"])
                {
                    auto velocityNode = node["Velocity"];
                    comp.velocity.x = velocityNode[0].as<float>();
                    comp.velocity.y = velocityNode[1].as<float>();
                }

                if (node["OnGround"])
                    comp.onGround = node["OnGround"].as<bool>();
            }
        }


        inline void DeserializeEntity(Entity entity, const YAML::Node& entityNode)
        {
            DeserializeTagComponent(entity, entityNode);
            DeserializeTransformComponent(entity, entityNode);
            DeserializeCameraComponent(entity, entityNode);
            DeserializeSpriteRendererComponent(entity, entityNode);
            DeserializeNativeScriptComponent(entity, entityNode);
            DeserializeBoxCollider2DComponent(entity, entityNode);
            DeserializeRigidBody2DComponent(entity, entityNode);
            DeserializeCircleRendererComponent(entity, entityNode);
            DeserializeCircleCollider2DComponent(entity, entityNode);
			DeserializeHealthComponent(entity, entityNode);
			DeserializeNPCAIMovementComponent(entity, entityNode);
			DeserializeNPCAIVisionComponent(entity, entityNode);
			DeserializeCharacterControllerComponent(entity, entityNode);
			DeserializeWeaponComponent(entity, entityNode);
        }


    }

    void SceneSerializer::Serialize(const std::string& filepath)
    {
        
        std::filesystem::path filePath(filepath);
        if (!std::filesystem::exists(filePath.parent_path()))
        {
            std::filesystem::create_directories(filePath.parent_path());
        }

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Scene" << YAML::Value << "My Scene"; // Customize scene metadata as needed.
        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

        // Iterate over all entities that have a TagComponent.
        // If every entity has a TagComponent, this iterates over all entities.
        auto view = m_scene->m_registry.view<TagComponent>();
        for (auto entityID : view)
        {
            Entity entity{ entityID, m_scene.get() };
            SerializeUtils::SerializeEntity(entity, out);
        }

        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::ofstream fout(filepath);
        fout << out.c_str();
    }



	void SceneSerializer::SerializeRuntime(const std::string& filepath)
	{
		//TODO
		EE_CORE_ASSERT(false);

	}




    bool SceneSerializer::Deserialize(const std::string& filepath)
    {
        if (!std::filesystem::exists(filepath))
        {
            EE_CORE_ERROR("Failed to open file: {}", filepath);
            return false;
        }

        std::ifstream stream(filepath);
        if (!stream)
        {
            EE_CORE_ERROR("Failed to open file: {}", filepath);
            EE_CORE_ASSERT(false, "Failed to open file");
            return false;
        }
        std::stringstream strStream;
        strStream << stream.rdbuf();

        YAML::Node data = YAML::Load(strStream.str());
        if (!data["Scene"])
            return false;

        std::string sceneName = data["Scene"].as<std::string>();
       // EE_CORE_TRACE("deserializing scene: '{0}' ", sceneName);
        auto entities = data["Entities"];
        if (entities)
        {
            for (const auto& entityNode : entities)
            {
                uint32_t entityID = entityNode["ID"].as<uint64_t>();
                Entity entity = m_scene->CreateEntityWithUUID(entityID);

                SerializeUtils::DeserializeEntity(entity, entityNode);
            }
        }

        return true;
    }

	bool SceneSerializer::DeserializeRuntime(const std::string& filepath)
	{
		//TODO
		EE_CORE_ASSERT(false);

		return false;
	}

}