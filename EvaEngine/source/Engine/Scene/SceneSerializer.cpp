#include "pch.h"
#include "SceneSerializer.h"

#include "Entity.h"     
#include "Component.h" 

#include <yaml-cpp/yaml.h>
#include <fstream>
#include <filesystem>

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

            // Serialize color
            out << YAML::Key << "Color" << YAML::Value << YAML::Flow
                << std::vector<float>{ sprite.Color.r, sprite.Color.g, sprite.Color.b, sprite.Color.a };

            // Serialize texture (if it exists)
            /* MAke asset manager
            if (sprite.Texture)
                out << YAML::Key << "Texture" << YAML::Value << sprite.Texture->GetPath();
            else
                out << YAML::Key << "Texture" << YAML::Value << ""; // Empty string if no texture

            */
            // Serialize tiling
            out << YAML::Key << "Tiling" << YAML::Value << sprite.Tiling;

            out << YAML::EndMap;
        }


        inline void SerializeNativeScriptComponent(Entity entity, YAML::Emitter& out)
        {
            // For demonstration, we simply output that a native script is attached.
            out << YAML::Key << "NativeScriptComponent" << YAML::Value << "Script Attached";
        }

        // Serializes an individual entity by checking for each component.
        void SerializeEntity(Entity entity, YAML::Emitter& out)
        {
            out << YAML::BeginMap;

            // You might want to serialize an ID; here we cast the entt::entity to a uint32_t.
            out << YAML::Key << "ID" << YAML::Value << static_cast<uint32_t>(entity);

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

            out << YAML::EndMap;
        }


        //************************* Deserialize ****************************************

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
                auto& sprite = entity.AddComponent<SpriteRendererComponent>();
                auto color = entityNode["SpriteRendererComponent"]["Color"].as<std::vector<float>>();
                sprite.Color = { color[0], color[1], color[2], color[3] };
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

        inline void DeserializeEntity(Entity entity, const YAML::Node& entityNode)
        {
            DeserializeTagComponent(entity, entityNode);
            DeserializeTransformComponent(entity, entityNode);
            DeserializeCameraComponent(entity, entityNode);
            DeserializeSpriteRendererComponent(entity, entityNode);
            DeserializeNativeScriptComponent(entity, entityNode);
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
            return false;

        std::ifstream stream(filepath);
        if (!stream)
            return false;

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
                uint32_t entityID = entityNode["ID"].as<uint32_t>();
                Entity entity = m_scene->CreateEntity();

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