#include "SceneHierarchyPanel.h"

#include "imgui/imgui.h"
#include "Engine/Scene/Component.h"
#include <glm/gtc/type_ptr.hpp>

//#include "entt.hpp"

namespace Engine {


	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
	{
		SetContext(context);
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& context)
	{
		m_context = context;
	}

    void SceneHierarchyPanel::OnImGuiRender()
    {
        ImGui::Begin("Scene Hierarchy");

        // Iterate over all entities with a TagComponent (assuming all entities have a name)
        m_context->m_registry.view<TagComponent>().each([&](auto entityID, TagComponent& tagComp)
            {
                Entity entity{ entityID, m_context.get() };

                DrawEntityNode(entity);
            });

        
        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
        {
            m_selectionContext = {};
        }

        ImGui::End();

        ImGui::Begin("Properties");

        if (m_selectionContext)
        {
            DrawComponents(m_selectionContext);
        }

        ImGui::End();

    }

    void SceneHierarchyPanel::DrawEntityNode(Entity entity)
    {
        auto& tagComp = entity.GetComponent<TagComponent>();

        ImGuiTreeNodeFlags flags = (m_selectionContext == entity ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tagComp.Tag.c_str());

        if (ImGui::IsItemClicked())
        {
            m_selectionContext = entity;  // Store the selected entity
        }

        if (opened)
        {
            ImGui::TreePop();
        }
    }

    void SceneHierarchyPanel::DrawComponents(Entity entity)
    {
        if (entity.HasComponent<TagComponent>())
        {
            auto& tag = entity.GetComponent<TagComponent>().Tag;



            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strcpy_s(buffer, sizeof(buffer), tag.c_str());
            if (ImGui::InputText("Tag", buffer, sizeof(buffer)))
            {
                tag = std::string(buffer);
            }

           // ImGui::Text("TagComponent: %s", tag.Tag.c_str());
        }

        if (entity.HasComponent<TransformComponent>())
        {
            if (ImGui::TreeNodeEx((void*)typeid(TransformComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Transform"))
            {
                auto& transform = entity.GetComponent<TransformComponent>().Transform;
                ImGui::DragFloat3("position", glm::value_ptr(transform[3]), 0.25f);
                ImGui::TreePop();
            }
        }

        if (entity.HasComponent<CameraComponent>())
        {
            if (ImGui::TreeNodeEx((void*)typeid(CameraComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Camera"))
            {
                auto& transform = entity.GetComponent<TransformComponent>().Transform;
                ImGui::DragFloat3("position", glm::value_ptr(transform[3]), 0.25f);

                auto& cameraComp = entity.GetComponent<CameraComponent>();
                auto& camera = cameraComp.Camera;
                
                ImGui::Checkbox("Primary", &cameraComp.Primary);

                const char* projectionTypeStrings[] = {" Perspective", " Orthographic" };
                const char* currentProjectionTypeString = projectionTypeStrings[(int)camera.GetProjectionType()];

                if (ImGui::BeginCombo("Projection", currentProjectionTypeString))
                {
                    const int projectionTypeCount = 2;
                    for (int i = 0; i < projectionTypeCount; i++)
                    {
                        bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
                        if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
                        {
                            // clicked new type
                            currentProjectionTypeString = projectionTypeStrings[i];
                            camera.SetProjectionType((SceneCamera::ProjectionType)i);
                        }

                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }

                    }
                    ImGui::EndCombo();
                }

                if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
                {
                    float orthoSize = camera.GetOrthographicSize();
                    if (ImGui::DragFloat("size", &orthoSize))
                    {
                        camera.SetOrthographicSize(orthoSize);
                    }

                    float orthoNear = camera.GetOrthographicNearClip();
                    if (ImGui::DragFloat("Near clip", &orthoNear))
                    {
                        camera.SetOrthographicNearClip(orthoNear);
                    }

                    float orthoFar = camera.GetOrthographicFarClip();
                    if (ImGui::DragFloat("Far Clip", &orthoFar))
                    {
                        camera.SetOrthographicFarClip(orthoFar);
                    }
                    ImGui::Checkbox("Fixed Aspect Ratio", &cameraComp.FixedAspectRatio);

                }

                if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
                {
                    float verticalFOV = glm::degrees(camera.GetPerspectiveFOV());
                    if (ImGui::DragFloat("FOV", &verticalFOV))
                    {
                        camera.SetPerspectiveFOV(glm::radians(verticalFOV));
                    }

                    float orthoNear = camera.GetPerspectiveNearClip();
                    if (ImGui::DragFloat("Near clip", &orthoNear))
                    {
                        camera.SetPerspectiveNearClip(orthoNear);
                    }

                    float orthoFar = camera.GetPerspectiveFarClip();
                    if (ImGui::DragFloat("Far Clip", &orthoFar))
                    {
                        camera.SetPerspectiveFarClip(orthoFar);
                    }
                }
                ImGui::TreePop();

            }
        }
        // Add checks for other components here...
    }



}

