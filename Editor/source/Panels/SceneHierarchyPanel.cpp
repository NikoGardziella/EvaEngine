#include "pch.h"

#include "SceneHierarchyPanel.h"
#include "Engine/AssetManager/AssetManager.h"
#include "Engine/Scene/Component.h"

#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"
#include <imgui/imgui_internal.h>
#include "ImGuizmo/ImGuizmo.h"

//#include "entt.hpp"

namespace Engine {

    extern const std::filesystem::path s_assetPath;


	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
	{
		SetEditorContext(context);
	}

	void SceneHierarchyPanel::SetEditorContext(const Ref<Scene>& context)
	{
        m_editorContext = context;
        m_selectionContext = {};

	}

    void SceneHierarchyPanel::SetGameContext(const Ref<Scene>& context)
    {
        m_gameContext = context;
        m_selectionContext = {};
    }

    void SceneHierarchyPanel::OnImGuiRender()
    {
        DrawContext();
        
    }


    static void DrawVec3Control(const std::string& label, glm::vec3& values, int& gizmoType, ImGuizmo::OPERATION gizmoOperation, float resetValue = 0.0f, float collumWidth = 100.0f)
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
        if (ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f"))
        {
            gizmoType = gizmoOperation;

        }
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
        if (ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f"))
        {
            gizmoType = gizmoOperation;
        }
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
        if (ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f"))
        {
            gizmoType = gizmoOperation;
        }
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();
        ImGui::Columns(1);

        ImGui::PopID();
    }


    void SceneHierarchyPanel::SetSelectedEntity(Entity entity)
    {
        m_selectionContext = entity;
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

        if (ImGui::BeginPopupContextItem())
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
            m_editorContext->DestroyEntity(entity);
            if (m_selectionContext == entity)
            {
                m_selectionContext = {};
            }
        }
    }

    void SceneHierarchyPanel::DrawContext()
    {
        ImGui::Begin("Scene Hierarchy");

        bool clickedOnEmptySpace = true;  // Track if no entity was clicked

        // Iterate over all entities with a TagComponent
        ImGui::PushID((void*)m_editorContext.get()); // Push scene ID to make entity IDs unique

        m_editorContext->m_registry.view<TagComponent>().each([&](auto entityID, TagComponent& tagComp)
            {
                Entity entity{ entityID, m_editorContext.get() };
               // EE_CORE_INFO("Entity: {0} ", entity.GetComponent<IDComponent>().ID);

                ImGui::PushID(entity.GetComponent<IDComponent>().ID);
                DrawEntityNode(entity);
                ImGui::PopID();
            });
        ImGui::PopID();


        ImGui::PushID((void*)m_gameContext.get()); // Push scene ID to make entity IDs unique

        m_gameContext->m_registry.view<TagComponent>().each([&](auto entityID, TagComponent& tagComp)
            {
                Entity entity{ entityID, m_gameContext.get() };
                // EE_CORE_INFO("Entity: {0} ", entity.GetComponent<IDComponent>().ID);

                ImGui::PushID(entity.GetComponent<IDComponent>().ID);
                DrawEntityNode(entity);
                ImGui::PopID();
            });
        ImGui::PopID();
       
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
                m_editorContext->CreateEntity("Empty entity");
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

    template<typename T, typename UIFunction>
    static void DrawComponent(const std::string& name, Entity entity, UIFunction function)
    {
        if (entity.HasComponent<T>())
        {
            auto& component = entity.GetComponent<T>();

            // Unique tree node flags
            const ImGuiTreeNodeFlags treeNodeFlags =
                ImGuiTreeNodeFlags_DefaultOpen |
                ImGuiTreeNodeFlags_AllowItemOverlap |
                ImGuiTreeNodeFlags_Framed |
                ImGuiTreeNodeFlags_FramePadding |
                ImGuiTreeNodeFlags_SpanAvailWidth;

            ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

            // Styling
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
            float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
            ImGui::Separator();

            // Push unique IDs to avoid duplication
            ImGui::PushID((int)entity.GetComponent<IDComponent>().ID);  // Entity-specific ID
            ImGui::PushID(typeid(T).hash_code());  // Component type-specific ID

            // Add component name and ID to create a unique tree node
            bool open = ImGui::TreeNodeEx((std::string("##Component_") + std::to_string((int)entity.GetComponent<IDComponent>().ID) + "_" + std::to_string(typeid(T).hash_code())).c_str(),
                treeNodeFlags, name.c_str());

            // Pop the IDs after drawing the node
            ImGui::PopID();
            ImGui::PopID();

            // Reset style
            ImGui::PopStyleVar();

            // Display button for component settings
            ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
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

            // If the tree node is open, draw the component using the provided function
            if (open)
            {
                function(component);
                ImGui::TreePop();
            }

            // Handle component removal
            if (removeComponent)
            {
                entity.RemoveComponent<T>();
            }
        }
    }


    void SceneHierarchyPanel::DrawComponents(Entity entity)
    {
        if (!entity)
            return;


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

        // move poput so its not in viweport( Would crash)
        ImVec2 buttonPos = ImGui::GetItemRectMin();
        ImVec2 popupPos = ImVec2(buttonPos.x - 25.0f, buttonPos.y + 15.0f); 

        // Set the new position for the popup
        ImGui::SetNextWindowPos(popupPos);


        if (ImGui::BeginPopup("Add component"))
        {


            if (!m_selectionContext.HasComponent<CameraComponent>())
            {
                if (ImGui::MenuItem("Camera"))
                {
                    m_selectionContext.AddComponent<CameraComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!m_selectionContext.HasComponent<SpriteRendererComponent>())
            {
                if (ImGui::MenuItem("Sprite renderer"))
                {
                    m_selectionContext.AddComponent<SpriteRendererComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!m_selectionContext.HasComponent<CircleRendererComponent>())
            {
                if (ImGui::MenuItem("Circle renderer"))
                {
                    m_selectionContext.AddComponent<CircleRendererComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!m_selectionContext.HasComponent<TransformComponent>())
            {
                if (ImGui::MenuItem("Transform"))
                {
                    m_selectionContext.AddComponent<TransformComponent>();
                    ImGui::CloseCurrentPopup();

                }

            }
            if (!m_selectionContext.HasComponent<RigidBody2DComponent>())
            {
                if (ImGui::MenuItem("RigidBody 2D"))
                {
                    m_selectionContext.AddComponent<RigidBody2DComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            if (ImGui::MenuItem("BoxCollider 2D"))
            {
                m_selectionContext.AddComponent<BoxCollider2DComponent>();
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Circle Collider"))
            {
                m_selectionContext.AddComponent<CircleCollider2DComponent>();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::PopItemWidth();


        DrawComponent<TransformComponent>("Transform", entity, [this](auto& component)
            {
                DrawVec3Control("Translation", component.Translation, m_guizmoType, ImGuizmo::OPERATION::TRANSLATE);
                 
                glm::vec3 rotation = glm::degrees(component.Rotation);
                DrawVec3Control("Rotation", rotation, m_guizmoType, ImGuizmo::OPERATION::ROTATE);

                component.Rotation = glm::radians(rotation);
                DrawVec3Control("Scale", component.Scale, m_guizmoType, ImGuizmo::OPERATION::SCALE);
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
                    if (ImGui::DragFloat("FOV", &verticalFOV, 0.1f, 1.0f, 179.0f)) // Clamped for stability
                    {
                        camera.SetPerspectiveFOV(glm::radians(verticalFOV));
                    }

                    float perspNear = camera.GetPerspectiveNearClip();
                    if (ImGui::DragFloat("Near Clip", &perspNear, 0.01f, 0.01f, camera.GetPerspectiveFarClip() - 0.1f)) // Near must be > 0
                    {
                        camera.SetPerspectiveNearClip(perspNear);
                    }

                    float perspFar = camera.GetPerspectiveFarClip();
                    if (ImGui::DragFloat("Far Clip", &perspFar, 1.0f, camera.GetPerspectiveNearClip() + 0.1f, 10000.0f)) // Far must be > Near
                    {
                        camera.SetPerspectiveFarClip(perspFar);
                    }
                }

            });

        DrawComponent<SpriteRendererComponent>("Sprite renderer", entity, [](auto& component)
            {
                ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));

                ImGui::Button("Texture", ImVec2(100.0f, 0.0f));

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                    {
                        const wchar_t* path = (const wchar_t*)payload->Data;
                        std::filesystem::path texturePath = std::filesystem::path(AssetManager::GetAssetFolderPath()) / path;

                        component.Texture = Texture2D::Create(texturePath.string());
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::DragFloat("Tiling", &component.Tiling, 0.1f, 0.0f, 100.0f);
            });

        DrawComponent<CircleRendererComponent>("Circle  renderer", entity, [](auto& component)
            {
                ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
                ImGui::DragFloat("Fade", &component.Fade, 0.00025f, 0.0f, 1.0f);
                ImGui::DragFloat("Thickness", &component.Thickness, 0.025f, 0.0f, 1.0f);
            });

        DrawComponent<CircleCollider2DComponent>("Circle  Collider", entity, [](auto& component)
            {
                ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset));
                ImGui::DragFloat("Radius", &component.Radius, 0.1f, 0.0f, 100.0f);
                ImGui::DragFloat("Density", &component.Density, 0.01, 0.0f, 1.0f);
                ImGui::DragFloat("Friction", &component.Friction, 0.01, 0.0f, 1.0f);
                ImGui::DragFloat("Restitution", &component.Restitution, 0.01, 0.0f, 1.0f);
                ImGui::DragFloat("Restitution Threshold", &component.RestitutionThershold, 0.01, 0.0f, 1.0f);
            });


        DrawComponent<RigidBody2DComponent>("Rigid Body2d", entity, [](auto& component)
            {
                const char* bodyTypeStrings[] = { " Static", " Dynamic", "Kinematic"};
                const char* currentBodyTypeString = bodyTypeStrings[(int)component.Type];

                if (ImGui::BeginCombo("Body Type", currentBodyTypeString))
                {
                    const int bodyTypeCount = 3;
                    for (int i = 0; i < bodyTypeCount; i++)
                    {
                        bool isSelected = currentBodyTypeString == bodyTypeStrings[i];
                        if (ImGui::Selectable(bodyTypeStrings[i], isSelected))
                        {
                            // clicked new type
                            currentBodyTypeString = bodyTypeStrings[i];
                            component.Type = (RigidBody2DComponent::BodyType)i;
                        }

                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }

                    }
                    ImGui::EndCombo();
                }

                

                ImGui::Checkbox("Fixed Rotation", &component.FixedRotation);
            });
      

        DrawComponent<BoxCollider2DComponent>("Box Collider 2D", entity, [](auto& component)
            {
                
                ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset));
                ImGui::DragFloat2("Size", glm::value_ptr(component.Size));
                ImGui::DragFloat("Density", &component.Density, 0.01, 0.0f, 1.0f);
                ImGui::DragFloat("Friction", &component.Friction, 0.01, 0.0f, 1.0f);
                ImGui::DragFloat("Restitution", &component.Restitution, 0.01, 0.0f, 1.0f);
                ImGui::DragFloat("Restitution Threshold", &component.RestitutionThershold, 0.01, 0.0f, 1.0f);

            });
        // Add checks for other components here...
    }



}

