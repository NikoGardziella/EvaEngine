#include "SceneHierarchyPanel.h"

#include "imgui/imgui.h"
#include "Engine/Scene/Component.h"
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui_internal.h>

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

        bool clickedOnEmptySpace = true;  // Track if no entity was clicked

        // Iterate over all entities with a TagComponent
        m_context->m_registry.view<TagComponent>().each([&](auto entityID, TagComponent& tagComp)
            {
                Entity entity{ entityID, m_context.get() };

                DrawEntityNode(entity);

                if (ImGui::IsItemHovered() || ImGui::IsItemClicked(ImGuiMouseButton_Right))
                {
                    clickedOnEmptySpace = false;
                }
            });

        // Reset selection if clicking on empty space
        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && clickedOnEmptySpace)
        {
            m_selectionContext = {};
        }

        // Open "Create Empty Entity" popup **only** if no entity was clicked
        if (clickedOnEmptySpace && ImGui::BeginPopupContextWindow("CreateEntityPopup", ImGuiPopupFlags_MouseButtonRight))
        {
            if (ImGui::MenuItem("Create Empty Entity"))
            {
                m_context->CreateEntity("Empty entity");
            }
            ImGui::EndPopup();
        }

        ImGui::End();

        // Properties panel
        ImGui::Begin("Properties");
        if (m_selectionContext)
        {
            DrawComponents(m_selectionContext);

            

        }
        ImGui::End();
    }


    static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float collumWidth = 100.0f)
    {
        ImGuiIO& io = ImGui::GetIO();
        auto boldFont = io.Fonts->Fonts[0];




        ImGui::PushID(label.c_str());

        ImGui::Columns(2);

        ImGui::SetColumnWidth(0, collumWidth);
        ImGui::Text(label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;

        ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.80f, 0.10f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.90f, 0.2f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.75f, 0.12f, 0.12f, 1.0f });

        ImGui::PushFont(boldFont);
        if (ImGui::Button("X", buttonSize))
        {
            values.x = resetValue;
        }
        ImGui::PopFont();

        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

      
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.8f, 0.3f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.9f, 0.3f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.8f, 0.2f, 1.0f });

        ImGui::PushFont(boldFont);
        if (ImGui::Button("Y", buttonSize))
        {
            values.y = resetValue;
        }
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.20f, 0.20f, 0.8f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.25f, 0.25f, 0.9f, 1.0f });
        
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.25f, 0.22f, 0.8f, 1.0f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Z", buttonSize))
        {
            values.z = resetValue;
        }
        ImGui::PopStyleColor(3);
        ImGui::PopFont();

        ImGui::SameLine();
        ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();
        ImGui::Columns(1);

        ImGui::PopID();
    }


    void SceneHierarchyPanel::DrawEntityNode(Entity entity)
    {
        auto& tagComp = entity.GetComponent<TagComponent>();

        ImGuiTreeNodeFlags flags = (m_selectionContext == entity ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
        
        bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tagComp.Tag.c_str());

        if (ImGui::IsItemClicked())
        {
            m_selectionContext = entity;  // Store the selected entity
        }

        bool entityDeleted = false;

        if (ImGui::BeginPopupContextItem()) // Corrected: Using ImGui::BeginPopupContextItem()
        {
            if (ImGui::MenuItem("Delete entity"))
            {
                entityDeleted = true;
            }
            ImGui::EndPopup(); 
        }

        if (opened)
        {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow;
            bool opened = ImGui::TreeNodeEx((void*)9213123, flags, tagComp.Tag.c_str());
            if (opened)
            {
                ImGui::TreePop();

            }
            ImGui::TreePop();
        }

        if (entityDeleted)
        {
            m_context->DestroyEntity(entity);
            if (m_selectionContext == entity)
            {
                m_selectionContext = {};
            }
        }
    }

    template<typename T, typename UIFunction>
    static void DrawComponent(const std::string& name, Entity entity, UIFunction function)
    {
        if (entity.HasComponent<T>())
        {
            auto& component = entity.GetComponent<T>();
            const ImGuiTreeNodeFlags treeNodeFlags =
                ImGuiTreeNodeFlags_DefaultOpen |
                ImGuiTreeNodeFlags_AllowItemOverlap |
                ImGuiTreeNodeFlags_Framed |
                ImGuiTreeNodeFlags_FramePadding |
                ImGuiTreeNodeFlags_SpanAvailWidth;
            ImVec2 contenRegionAvailable = ImGui::GetContentRegionAvail();


            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4 ,4 });
            float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
            ImGui::Separator();

            bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
            ImGui::PopStyleVar();
            
            ImGui::SameLine(contenRegionAvailable.x - lineHeight * 0.5f);
            if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
            {
                ImGui::OpenPopup("Component settings");

            }

            bool removeComponent = false;
            if (ImGui::BeginPopup("Component settings"))
            {
                if (ImGui::MenuItem("Remove component"))
                {
                    removeComponent = true;
                }
                ImGui::EndPopup();
            }



            if (open)
            {
                function(component);
                ImGui::TreePop();
            }

            if (removeComponent)
            {
                entity.RemoveComponent<T>();
            }
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
            if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
            {
                tag = std::string(buffer);
            }

           // ImGui::Text("TagComponent: %s", tag.Tag.c_str());
        }

        ImGui::SameLine();
        ImGui::PushItemWidth(-1);

        if (ImGui::Button("Add component"))
        {
            ImGui::OpenPopup("Add component");
        }

        if (ImGui::BeginPopup("Add component"))
        {

            if (ImGui::MenuItem("Camera"))
            {
                m_selectionContext.AddComponent<CameraComponent>();
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Sprite renderer"))
            {
                m_selectionContext.AddComponent<SpriteRendererComponent>();
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Transform"))
            {
                m_selectionContext.AddComponent<TransformComponent>();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();

        }
        ImGui::PopItemWidth();


        DrawComponent<TransformComponent>("Transform", entity, [](auto& component)
            {
                DrawVec3Control("Translation", component.Translation);

                glm::vec3 rotation = glm::degrees(component.Rotation);
                DrawVec3Control("Rotation", rotation);
                component.Rotation = glm::radians(rotation);
                DrawVec3Control("Scale", component.Scale);
            });

        DrawComponent<CameraComponent>("Camera", entity, [](auto& component)
            {
                auto& cameraComp = component;
                auto& camera = component.Camera;

                ImGui::Checkbox("Primary", &cameraComp.Primary);

                const char* projectionTypeStrings[] = { " Perspective", " Orthographic" };
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
            });

        DrawComponent<SpriteRendererComponent>("Sprite renderer", entity, [](auto& component)
            {
                ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
            });

      



        // Add checks for other components here...
    }



}

