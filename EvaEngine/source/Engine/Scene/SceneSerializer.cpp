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
#include "Components/Render/TileComponent.h"
#include <Engine/Map/TextureStreaming/TextureStreamingSystem.h>
#include <string>
#include "Components/Vehicles/VehicleComponent.h"
#include <Engine/Core/Assert.h>
#include "Engine/Scene/Components/Map/AreaComponent.h"
#include "Components/Light/DirectionalLightComponent.h"
#include <Engine/Map/Tile/TileDefinitionRegistry.h>

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
        /*
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

        */
        inline void SerializeNPCAIVisionComponent(Entity entity, YAML::Emitter& out)
        {
            const NPCAIVisionComponent& comp = entity.GetComponent<NPCAIVisionComponent>();

            out << YAML::Key << "NPCAIVisionComponent";
            out << YAML::BeginMap;

            out << YAML::Key << "ViewRadius" << YAML::Value << comp.ViewRadius;
            out << YAML::Key << "ViewAngle" << YAML::Value << comp.ViewAngle;
            out << YAML::Key << "HasLineOfSight" << YAML::Value << comp.hasLOS;

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
        inline void SerializeTileComponent(Entity entity, YAML::Emitter& out)
        {
            const auto& comp = entity.GetComponent<TileComponent>();

            out << YAML::Key << "TileComponent";
            out << YAML::BeginMap;

            out << YAML::Key << "TileID" << YAML::Value << comp.TileID;

           // out << YAML::Key << "WorldPos" << YAML::Value << YAML::Flow
          //      << std::vector<float>{ comp.WorldPos.x, comp.WorldPos.y };

            // Optional texture name
            if (comp.Texture && !comp.Texture->GetName().empty())
                out << YAML::Key << "Texture" << YAML::Value << comp.Texture->GetName();
            else
                out << YAML::Key << "Texture" << YAML::Value << "";

            // Serialize all tiles in the group
            out << YAML::Key << "Tiles" << YAML::Value << YAML::BeginSeq;
            for (const auto& tile : comp.tiles)
            {
                out << YAML::BeginMap;

                out << YAML::Key << "Position" << YAML::Value << YAML::Flow
                    << std::vector<float>{ tile.position.x, tile.position.y };

                out << YAML::Key << "UV" << YAML::Value << YAML::Flow
                    << std::vector<float>{ tile.UV.x, tile.UV.y, tile.UV.z, tile.UV.w };

                out << YAML::Key << "Name" << YAML::Value << tile.name;
                out << YAML::Key << "IsDestructible" << YAML::Value << tile.IsDestructible;
                out << YAML::Key << "IsSupportingRoof" << YAML::Value << tile.IsSupportingRoof;
                out << YAML::Key << "IsRoof" << YAML::Value << tile.IsRoof;
                out << YAML::Key << "Category" << YAML::Value << ToString(tile.Category);
                out << YAML::Key << "Material" << YAML::Value << ToString(tile.Material);
                out << YAML::Key << "Health" << YAML::Value << tile.TileHealth;
                out << YAML::Key << "UID" << YAML::Value << tile.UID;
                out << YAML::Key << "TileDirection" << YAML::Value << TileDirectionToString(tile.TileDirection);

                out << YAML::EndMap;
            }
            out << YAML::EndSeq;

            out << YAML::EndMap;
        }

        inline void SerializeVehicleComponent(Entity entity, YAML::Emitter& out)
        {
            if (!entity.HasComponent<VehicleComponent>())
                return;

            const auto& comp = entity.GetComponent<VehicleComponent>();

            out << YAML::Key << "VehicleComponent" << YAML::Value;
            out << YAML::BeginMap;

            out << YAML::Key << "Velocity" << YAML::Value << YAML::Flow << YAML::BeginSeq << comp.Velocity.x << comp.Velocity.y << YAML::EndSeq;
            out << YAML::Key << "Mass" << YAML::Value << comp.Mass;

           
            out << YAML::EndMap;
        }

        inline void SerializeAreaComponent(Entity entity, YAML::Emitter& out)
        {
            if (!entity.HasComponent<AreaComponent>())
                return;

            const auto& comp = entity.GetComponent<AreaComponent>();

            out << YAML::Key << "AreaComponent" << YAML::Value;
            out << YAML::BeginMap;

            out << YAML::Key << "Min" << YAML::Value << YAML::Flow << YAML::BeginSeq << comp.Min.x << comp.Min.y << YAML::EndSeq;
            out << YAML::Key << "Max" << YAML::Value << YAML::Flow << YAML::BeginSeq << comp.Max.x << comp.Max.y << YAML::EndSeq;
            out << YAML::Key << "Id" << YAML::Value << comp.Id;
            out << YAML::Key << "Type" << YAML::Value << (uint32_t)comp.Type;


            out << YAML::EndMap;
        }

        static bool IgnoreThisEntity(Entity entity)
        {
            // dont serialize if entity has any of these components

            if (entity.HasComponent<DirectionalLightComponent>())
            {
                return true;
            }
            if (entity.HasComponent<TileComponent>())
            {
                // use compact tiles for saving
                return true;
            }
            if (!entity.HasComponent<TransformComponent>())
            {
                // ignore all withou ttransform
                return true;
            }

            return false;
        }

        // Serializes an individual entity by checking for each component.
        void SerializeEntity(Entity entity, YAML::Emitter& out)
        {
            EE_CORE_ASSERT(entity.HasComponent<IDComponent>());

            if (IgnoreThisEntity(entity))
            {
                return;

            }


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
           // if (entity.HasComponent<NPCAIMovementComponent>())
           //     SerializeNPCAIMovementComponent(entity, out);
            if (entity.HasComponent<NPCAIVisionComponent>())
                SerializeNPCAIVisionComponent(entity, out);
            if (entity.HasComponent<CharacterControllerComponent>())
                SerializeCharacterControllerComponent(entity, out);
            if (entity.HasComponent<WeaponComponent>())
                SerializeWeaponComponent(entity, out);
            if (entity.HasComponent<TileComponent>())
                SerializeTileComponent(entity, out);
            if (entity.HasComponent<VehicleComponent>())
                SerializeVehicleComponent(entity, out);
            if (entity.HasComponent<AreaComponent>())
                SerializeAreaComponent(entity, out);




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
        
        inline void DeserializeTileComponent(Entity entity, const YAML::Node& entityNode)
        {
            if (!entityNode["TileComponent"])
                return;

            const auto& tc = entityNode["TileComponent"];

            TileComponent& tileComp = entity.AddComponent<TileComponent>();
            tileComp.TileID = tc["TileID"].as<uint32_t>();
          //  tileComp.WorldPos = glm::vec2(tc["WorldPos"][0].as<float>(), tc["WorldPos"][1].as<float>());

            // Load texture name (optional)
            std::string textureName = tc["Texture"] ? tc["Texture"].as<std::string>() : "";
            if (!textureName.empty())
            {
                tileComp.Texture = AssetManager::GetTexture(textureName); // Adjust if using another loader
            }
            else
            {
                tileComp.Texture = nullptr;
            }

            // Load tiles
            if (tc["Tiles"])
            {
                for (const auto& tileNode : tc["Tiles"])
                {
                    TileInfo tile;
                    tile.position = glm::vec2(tileNode["Position"][0].as<float>(), tileNode["Position"][1].as<float>());
                    tile.name = tileNode["Name"] ? tileNode["Name"].as<std::string>() : "";
                    tile.UV = AssetManager::GetTileProperties(tile.name).uv;
                    tile.IsDestructible = tileNode["IsDestructible"] ? tileNode["IsDestructible"].as<bool>() : false;
                    tile.IsSupportingRoof = tileNode["IsSupportingRoof"] ? tileNode["IsSupportingRoof"].as<bool>() : false;
                    tile.IsRoof = tileNode["IsRoof"] ? tileNode["IsRoof"].as<bool>() : false;
                    tile.Category = CategoryFromString(tileNode["Category"].as<std::string>());
                    tile.Material = MaterialFromString(tileNode["Material"].as<std::string>());
                    tile.TileDirection = TileDirectionFromString(tileNode["TileDirection"].as<std::string>());
                    tile.TileHealth = tileNode["Health"].as<uint32_t>();
                    tile.UID = tileNode["UID"].as<uint64_t>();
                    tileComp.tiles.push_back(tile);
                }
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
        /*
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
        */

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
                    comp.hasLOS = node["HasLineOfSight"].as<int>();

                // VisibleTarget is runtime/internal, not deserialized
                comp.VisibleTarget = Entity{};
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
                auto rotationDeg = entityNode["TransformComponent"]["Rotation"].as<std::vector<float>>();
                auto scale = entityNode["TransformComponent"]["Scale"].as<std::vector<float>>();

                transform.Translation = { translation[0], translation[1], translation[2] };

                // YAML stored in degrees -> convert to radians for engine
                glm::vec3 rotDeg(rotationDeg[0], rotationDeg[1], rotationDeg[2]);
                transform.Rotation = glm::radians(rotDeg);

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
        
        inline void DeserializeSpriteRendererComponent(Entity entity, const YAML::Node& entityNode, Ref<Scene> scene)
        {
            if (entityNode["SpriteRendererComponent"])
            {
                SpriteRendererComponent& sprite = entity.AddComponent<SpriteRendererComponent>();
                auto color = entityNode["SpriteRendererComponent"]["Color"].as<std::vector<float>>();
                sprite.Color = { color[0], color[1], color[2], color[3] };

				//sprite.Texture = AssetManager::CloneTexture(entityNode["SpriteRendererComponent"]["Texture"].as<std::string>());
                if (entity.HasComponent<SpriteRendererComponent>())
                {
                    bool isStatic = true;

                    if (entity.HasComponent<CharacterControllerComponent>() ||
                        entity.HasComponent<NPCAIVisionComponent>())
                    {
                        isStatic = false; // CharacterControllerComponent dynamic entity
                    }

                    if (isStatic)
                    {
                        std::vector<uint8_t> pixelData;
                        std::vector<uint8_t> healthData;
                        int width, height;
                        std::string TextureName = entityNode["SpriteRendererComponent"]["Texture"].as<std::string>();
                        if (AssetManager::GetTexturePixelData(TextureName, pixelData, healthData, width, height))
                        {
                            
                               scene->GetTextureStreamingSystem().UploadToChunkFromTexture(
                                entity.GetComponent<TransformComponent>().Translation,
                                entity.GetComponent<IDComponent>().ID, TextureName,
                                pixelData, healthData, width, height);
                        }
                    }
                    else
                    {
                        SpriteRendererComponent& renderComp = entity.GetComponent<SpriteRendererComponent>();
                        renderComp.Texture = AssetManager::CloneTexture(entityNode["SpriteRendererComponent"]["Texture"].as<std::string>());

                        auto color = entityNode["SpriteRendererComponent"]["Color"].as<std::vector<float>>();
                        renderComp.Color = { color[0], color[1], color[2], color[3] };


                    }
                }

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

        inline void DeserializeVehicleComponent(Entity entity, const YAML::Node& entityNode)
        {
            if (entityNode["VehicleComponent"])
            {
                VehicleComponent& component = entity.AddComponent<VehicleComponent>();
                const auto& node = entityNode["VehicleComponent"];

                if (node["Velocity"])
                {
                    auto velocityNode = node["Velocity"];
                    component.Velocity.x = velocityNode[0].as<float>();
                    component.Velocity.y = velocityNode[1].as<float>();
                }

                if (node["Mass"])
                    component.Mass = node["Mass"].as<uint32_t>();

            }
        }
        inline void DeserializeAreaComponent(Entity entity, const YAML::Node& entityNode)
        {
            if (entityNode["AreaComponent"])
            {
                AreaComponent& component = entity.AddComponent<AreaComponent>();
                const auto& node = entityNode["AreaComponent"];

                if (node["Min"])
                {
                    auto velocityNode = node["Min"];
                    component.Min.x = velocityNode[0].as<float>();
                    component.Min.y = velocityNode[1].as<float>();
                }
                if (node["Max"])
                {
                    auto velocityNode = node["Max"];
                    component.Max.x = velocityNode[0].as<float>();
                    component.Max.y = velocityNode[1].as<float>();
                }
                if (node["Id"])
                {

                    component.Id = node["Id"].as<uint32_t>();
                }
                if (node["Type"])
                {

                    component.Type = (AreaType)node["Type"].as<uint32_t>();
                }
            }
        }

        inline void DeserializeEntity(Entity entity, const YAML::Node& entityNode, Ref<Scene> scene)
        {
            DeserializeTagComponent(entity, entityNode);
            DeserializeTransformComponent(entity, entityNode);
            DeserializeCameraComponent(entity, entityNode);
            DeserializeNativeScriptComponent(entity, entityNode);
            DeserializeBoxCollider2DComponent(entity, entityNode);
            DeserializeRigidBody2DComponent(entity, entityNode);
            DeserializeCircleRendererComponent(entity, entityNode);
            DeserializeCircleCollider2DComponent(entity, entityNode);
			DeserializeHealthComponent(entity, entityNode);
			//DeserializeNPCAIMovementComponent(entity, entityNode);
			DeserializeNPCAIVisionComponent(entity, entityNode);
			DeserializeCharacterControllerComponent(entity, entityNode);
			DeserializeWeaponComponent(entity, entityNode);
			DeserializeTileComponent(entity, entityNode);
            DeserializeVehicleComponent(entity, entityNode);
            DeserializeAreaComponent(entity, entityNode);

            DeserializeSpriteRendererComponent(entity, entityNode, scene);
        }


    }


    inline void SerializeTileDefinitions(Ref<Scene> scene, YAML::Emitter& out)
    {
        const TileDefinitionRegistry& defs = AssetManager::GetTileDefinitions();

        for (const auto& [typeId, def] : defs.GetDefinitions())
        {
            out << YAML::BeginMap;

            out << YAML::Key << "TypeId" << YAML::Value << def.TypeId;
            out << YAML::Key << "Name" << YAML::Value << def.Name;
            out << YAML::Key << "UV" << YAML::Value << YAML::Flow
                << std::vector<float>{ def.UV.x, def.UV.y, def.UV.z, def.UV.w };

            out << YAML::Key << "Category" << YAML::Value << ToString(def.Category);
            out << YAML::Key << "Direction" << YAML::Value << TileDirectionToString(def.Direction);
            out << YAML::Key << "Material" << YAML::Value << ToString(def.Material);
            out << YAML::Key << "BaseHealth" << YAML::Value << def.BaseHealth;
            out << YAML::Key << "IsDestructible" << YAML::Value << def.IsDestructible;
            out << YAML::Key << "IsSupportingRoof" << YAML::Value << def.IsSupportingRoof;
            out << YAML::Key << "IsRoof" << YAML::Value << def.IsRoof;

            out << YAML::EndMap;
        }

    }


    inline void SerializeCompactGroups(Ref<Scene> scene, YAML::Emitter& out)
    {
        const CompactTileMap& compactMap = scene->GetCompactTileMap();

        out << YAML::Key << "CompactGroups" << YAML::Value << YAML::BeginSeq;

        for (const auto& [groupId, info] : compactMap.GetAllGroupInfo())
        {
            out << YAML::BeginMap;
            out << YAML::Key << "GroupId" << YAML::Value << groupId;
            out << YAML::Key << "GroupName" << YAML::Value << info.Name;
            out << YAML::Key << "OriginCell" << YAML::Value << YAML::Flow
                << std::vector<int>{ info.OriginCell.x, info.OriginCell.y };
            out << YAML::EndMap;
        }

        out << YAML::EndSeq;
    }

    inline void SerializeCompactTiles(Ref<Scene> scene, YAML::Emitter& out)
    {
        const CompactTileMap& compactMap = scene->GetCompactTileMap();

        for (const auto& [chunkCoord, chunk] : compactMap.GetChunks())
        {
            const glm::ivec2 chunkOriginCell = ChunkCoordToWorldOrigin(chunkCoord);

            for (int y = 0; y < TILE_CHUNK_H; ++y)
            {
                for (int x = 0; x < TILE_CHUNK_W; ++x)
                {
                    const glm::ivec2 worldCell = chunkOriginCell + glm::ivec2(x, y);

                    const std::vector<CompactTile>* tiles = compactMap.GetTiles(worldCell);
                    if (!tiles || tiles->empty())
                        continue;

                    for (const CompactTile& tile : *tiles)
                    {
                        if (tile.IsEmpty())
                            continue;

                        out << YAML::BeginMap;

                        out << YAML::Key << "Cell" << YAML::Value << YAML::Flow
                            << std::vector<int>{ worldCell.x, worldCell.y };

                        out << YAML::Key << "TypeId" << YAML::Value << tile.TypeId;
                        out << YAML::Key << "Flags" << YAML::Value << (int)tile.Flags;
                        out << YAML::Key << "Aux" << YAML::Value << (int)tile.Aux;
                        out << YAML::Key << "GroupId" << YAML::Value << tile.GroupId;

                        out << YAML::EndMap;
                    }
                }
            }
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
        out << YAML::BeginMap; // Start Scene Map
        out << YAML::Key << "Scene" << YAML::Value << "My Scene";

        // --- 1. ENTITIES SECTION ---
        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
        auto view = m_scene->m_registry.view<TagComponent>();
        for (auto entityID : view)
        {
            Entity entity{ entityID, m_scene.get() };
            SerializeUtils::SerializeEntity(entity, out);
        }
        out << YAML::EndSeq; // Close Entities Sequence

        /*
        // --- 2. TILE DEFINITIONS SECTION ---
        out << YAML::Key << "TileDefinitions" << YAML::Value << YAML::BeginSeq;
        SerializeTileDefinitions(m_scene, out);
        out << YAML::EndSeq; // Close TileDefinitions Sequence
        */

        // --- 4. COMPACT GROUP SECTION ---
        SerializeCompactGroups(m_scene, out);


        // --- 5. COMPACT TILES SECTION ---
        out << YAML::Key << "CompactTiles" << YAML::Value << YAML::BeginSeq;
        SerializeCompactTiles(m_scene, out);
        out << YAML::EndSeq; // Close CompactTiles Sequence

        out << YAML::EndMap; // Close Scene Map

        std::ofstream fout(filepath);
        fout << out.c_str();
    }


	void SceneSerializer::SerializeRuntime(const std::string& filepath)
	{
		//TODO
		EE_CORE_ASSERT(false);

	}


    inline void DeserializeTileDefinitions(Scene& scene, const YAML::Node& defsNode)
    {
        TileDefinitionRegistry& defs = AssetManager::GetTileDefinitions();

        for (const auto& defNode : defsNode)
        {
            TileDefinition def{};
            TileTypeKey key{};

            def.TypeId = defNode["TypeId"].as<uint16_t>();
            def.Name = defNode["Name"].as<std::string>();

            if (auto uvNode = defNode["UV"])
            {
                def.UV.x = uvNode[0].as<float>();
                def.UV.y = uvNode[1].as<float>();
                def.UV.z = uvNode[2].as<float>();
                def.UV.w = uvNode[3].as<float>();
            }

            def.Category = CategoryFromString(defNode["Category"].as<std::string>());
            def.Direction = TileDirectionFromString(defNode["Direction"].as<std::string>());
            def.Material = MaterialFromString(defNode["Material"].as<std::string>());
            def.BaseHealth = defNode["BaseHealth"].as<uint16_t>();
            def.IsDestructible = defNode["IsDestructible"].as<bool>();
            def.IsSupportingRoof = defNode["IsSupportingRoof"].as<bool>();
            def.IsRoof = defNode["IsRoof"].as<bool>();

            key.name = def.Name;
            key.uv = def.UV;
            key.category = def.Category;
            key.direction = def.Direction;

            defs.Register(def, key);
        }
    }


    inline void DeserializeCompactGroups(Scene& scene, const YAML::Node& groupsNode)
    {
        CompactTileMap& compactMap = scene.GetCompactTileMap();

        for (const auto& groupNode : groupsNode)
        {
            if (!groupNode["GroupId"] || !groupNode["OriginCell"])
                continue;

            const uint64_t groupId = groupNode["GroupId"].as<uint64_t>();

            glm::ivec2 originCell{};
            originCell.x = groupNode["OriginCell"][0].as<int>();
            originCell.y = groupNode["OriginCell"][1].as<int>();

            compactMap.SetGroupOrigin(groupId, originCell);


            const std::string groupName = groupNode["GroupName"].as<std::string>();
            compactMap.SetGroupName(groupId, groupName);
        }
    }

    inline void DeserializeCompactTiles(Scene& scene, const YAML::Node& compactTilesNode)
    {
        CompactTileMap& compactMap = scene.GetCompactTileMap();

        for (const auto& tileNode : compactTilesNode)
        {
            glm::ivec2 cell{};

            if (auto cellNode = tileNode["Cell"])
            {
                cell.x = cellNode[0].as<int>();
                cell.y = cellNode[1].as<int>();
            }
            else
            {
                continue;
            }

            CompactTile tile{};
            tile.TypeId = tileNode["TypeId"].as<uint16_t>();
            // tile.Flags = static_cast<uint8_t>(tileNode["Flags"].as<int>());
            tile.Flags = CompactTileFlags::None;
            tile.Aux = static_cast<uint8_t>(tileNode["Aux"].as<int>());
            tile.GroupId = tileNode["GroupId"].as<uint64_t>();

            if (tile.IsEmpty())
                continue;

            // Avoid duplicate same TypeId in same cell
            if (compactMap.HasTileType(cell, tile.TypeId))
            {
                EE_CORE_WARN(
                    "DeserializeCompactTiles: tile type {} already exists at cell ({}, {})",
                    tile.TypeId, cell.x, cell.y);
                continue;
            }

            compactMap.SetGroupOrigin(tile.GroupId, cell);

            compactMap.AddTile(cell, tile);
            compactMap.RegisterCellForGroup(tile.GroupId, cell);
            compactMap.MarkChunkDirtyForCell(cell);
        }
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

        // 1) Entities
        auto entities = data["Entities"];
        if (entities)
        {
            for (const auto& entityNode : entities)
            {
                uint64_t entityID = entityNode["ID"].as<uint64_t>();
                Entity entity = m_scene->CreateEntityWithUUID(entityID);

                SerializeUtils::DeserializeEntity(entity, entityNode, m_scene);
            }
        }

        /*
        // 2) TileDefinitions
        auto defsNode = data["TileDefinitions"];
        if (defsNode)
        {
            DeserializeTileDefinitions(*m_scene, defsNode);
        }

        */

        auto compactGroupInfoNode = data["CompactGroups"];

        DeserializeCompactGroups(*m_scene, compactGroupInfoNode);

        // 3) CompactTiles


        auto compactTilesNode = data["CompactTiles"];
        if (compactTilesNode)
        {
            DeserializeCompactTiles(*m_scene, compactTilesNode);
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