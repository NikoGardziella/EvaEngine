#include "pch.h"

#include "SceneHierarchyPanel.h"
#include "Engine/AssetManager/AssetManager.h"
#include "Engine/Scene/Component.h"

#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"
#include <imgui/imgui_internal.h>
#include "ImGuizmo/ImGuizmo.h"
#include <Engine/Scene/Components/Combat/HealthComponent.h>
#include <Engine/Scene/Components/NPC/NpcAIComponent.h>

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

    void SceneHierarchyPanel::SetNewComponentsContext(const Ref<Scene>& context)
    {
        m_newComponentsContext = context;
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
            if (!m_newComponentsContext->DestroyEntity(entity))
            {
                // entity was not in new components. Delete it from GameScene
                m_gameContext->DestroyEntity(entity);

            }

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

        
        /*
        ImGui::PushID((void*)m_newComponentsContext.get()); // Push scene ID to make entity IDs unique
        m_newComponentsContext->m_registry.view<TagComponent>().each([&](auto entityID, TagComponent& tagComp)
            {
                Entity entity{ entityID, m_newComponentsContext.get() };
               // EE_CORE_INFO("Entity: {0} ", entity.GetComponent<IDComponent>().ID);

                ImGui::PushID(entity.GetComponent<IDComponent>().ID);
                DrawEntityNode(entity);
                ImGui::PopID();
            });
        ImGui::PopID();
        
        
        */


        
        ImGui::PushID((void*)m_gameContext.get()); // Push scene ID to make entity IDs unique

        auto view = m_gameContext->m_registry.view<TagComponent>();
        std::vector<entt::entity> entityList(view.begin(), view.end());
        std::reverse(entityList.begin(), entityList.end());

        for (auto entityID : entityList)
        {
            Entity entity{ entityID, m_gameContext.get() };
            ImGui::PushID(entity.GetComponent<IDComponent>().ID);
            DrawEntityNode(entity);
            ImGui::PopID();
        }
        ImGui::PopID();
        
       
        // Reset selection if clicking on empty space
        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && clickedOnEmptySpace)
        {
            m_selectionContext = {};
        }

        // Open "Create Empty Entity" popup **only** if no entity was clicked
        if (!m_selectionContext && ImGui::BeginPopupContextWindow("CreateEntityPopup", ImGuiPopupFlags_MouseButtonRight))
        {
            if (ImGui::MenuItem("Create Empty Entity"))
            {
                // add new component to both of the registries. m_gameContext for normal rendering and interaciton
                // m_newComponentsContext is used to save the scene and avoid any of the entities created in game scene 
                // through code to be inclued in the saved scene file
                Entity newEntity = m_newComponentsContext->CreateEntity("Empty entity");
                m_gameContext->CreateEntityWithUUID(newEntity.GetUUID(), "Empty Entity");

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
    static void DrawComponent(const std::string& name, Entity entity, Scene* sceneContext, UIFunction function)
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
            ImGui::PushID((int)entity.GetComponent<IDComponent>().ID);
            ImGui::PushID(typeid(T).hash_code());

            bool open = ImGui::TreeNodeEx(
                (std::string("##Component_") + std::to_string((int)entity.GetComponent<IDComponent>().ID) + "_" + std::to_string(typeid(T).hash_code())).c_str(),
                treeNodeFlags, name.c_str());

            ImGui::PopID();
            ImGui::PopID();
            ImGui::PopStyleVar();

            ImGui::PushID(entity.GetComponent<IDComponent>().ID);
            ImGui::PushID(typeid(T).hash_code());

            
            ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
            if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
            {
                ImGui::OpenPopup("Component settings");
            }

            ImVec2 buttonPos = ImGui::GetItemRectMin();
            ImVec2 popupPos = ImVec2(buttonPos.x - 80.0f, buttonPos.y + 20.0f);

            ImGui::SetNextWindowPos(popupPos);
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

                if (sceneContext)
                {
                    // Find and remove entity from the other registry
                    entt::entity otherEntityID = Scene::GetEntityByUUID(sceneContext->GetRegistry(), entity.GetComponent<IDComponent>().ID);
                    if (otherEntityID != entt::null)
                    {
                        Entity otherEntity{ otherEntityID, sceneContext };
                        if (otherEntity.HasComponent<T>())
                        {
                            otherEntity.RemoveComponent<T>();
                        }
                    }
                }
            }

            ImGui::PopID();
            ImGui::PopID();
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
                m_newComponentsContext->GetRegistry().get<TagComponent>(entity).Tag = tag;
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

                    // I add new components for the game registry. I also want to add it to newcomponetsContext, which
                    // will be used for saving scene. Here I look for the corresponding entity - with UUID - from another scene
                    // add add component to it 
                    Entity entity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), m_selectionContext.GetComponent<IDComponent>().ID), m_newComponentsContext.get() };
                    if (entity)
                    {
                        m_newComponentsContext->GetRegistry().emplace<CameraComponent>(entity);
                    }
                }
            }
            if (!m_selectionContext.HasComponent<SpriteRendererComponent>())
            {
                if (ImGui::MenuItem("Sprite renderer"))
                {
                    m_selectionContext.AddComponent<SpriteRendererComponent>();
                    ImGui::CloseCurrentPopup();
                    Entity entity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), m_selectionContext.GetComponent<IDComponent>().ID), m_newComponentsContext.get() };
                    if (entity)
                    {
                        m_newComponentsContext->GetRegistry().emplace<SpriteRendererComponent>(entity);

                    }
                }
            }
            if (!m_selectionContext.HasComponent<CircleRendererComponent>())
            {
                if (ImGui::MenuItem("Circle renderer"))
                {
                    m_selectionContext.AddComponent<CircleRendererComponent>();
                    ImGui::CloseCurrentPopup();
                    Entity entity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), m_selectionContext.GetComponent<IDComponent>().ID), m_newComponentsContext.get() };
                    if (entity)
                    {
                        m_newComponentsContext->GetRegistry().emplace<CircleRendererComponent>(entity);

                    }
                }
            }
            if (!m_selectionContext.HasComponent<TransformComponent>())
            {
                if (ImGui::MenuItem("Transform"))
                {
                    m_selectionContext.AddComponent<TransformComponent>();
                    ImGui::CloseCurrentPopup();
                    Entity entity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), m_selectionContext.GetComponent<IDComponent>().ID), m_newComponentsContext.get()};
                    if (entity)
                    {
                        m_newComponentsContext->GetRegistry().emplace<TransformComponent>(entity);

                    }

                }

            }
            if (!m_selectionContext.HasComponent<RigidBody2DComponent>())
            {
                if (ImGui::MenuItem("RigidBody 2D"))
                {
                    m_selectionContext.AddComponent<RigidBody2DComponent>();
                    ImGui::CloseCurrentPopup();
                    Entity entity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), m_selectionContext.GetComponent<IDComponent>().ID), m_newComponentsContext.get() };
                    if (entity)
                    {
                        m_newComponentsContext->GetRegistry().emplace<RigidBody2DComponent>(entity);

                    }
                }
            }

            if (ImGui::MenuItem("BoxCollider 2D"))
            {
                m_selectionContext.AddComponent<BoxCollider2DComponent>();
                ImGui::CloseCurrentPopup();
                Entity entity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), m_selectionContext.GetComponent<IDComponent>().ID), m_newComponentsContext.get() };
                if (entity)
                {
                    m_newComponentsContext->GetRegistry().emplace<BoxCollider2DComponent>(entity);

                }
            }

            if (ImGui::MenuItem("Circle Collider"))
            {
                m_selectionContext.AddComponent<CircleCollider2DComponent>();
                ImGui::CloseCurrentPopup();
                Entity entity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), m_selectionContext.GetComponent<IDComponent>().ID), m_newComponentsContext.get() };
                if (entity)
                {
                    m_newComponentsContext->GetRegistry().emplace<CircleCollider2DComponent>(entity);

                }
            }
            if (ImGui::MenuItem("Health Component"))
            {
                m_selectionContext.AddComponent<HealthComponent>();
                ImGui::CloseCurrentPopup();
                Entity entity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), m_selectionContext.GetComponent<IDComponent>().ID), m_newComponentsContext.get() };
                if (entity)
                {
                    m_newComponentsContext->GetRegistry().emplace<HealthComponent>(entity);

                }
            }
            if (ImGui::MenuItem("NPC movement Component"))
            {
                m_selectionContext.AddComponent<NPCAIMovementComponent>();
                ImGui::CloseCurrentPopup();
                Entity entity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), m_selectionContext.GetComponent<IDComponent>().ID), m_newComponentsContext.get() };
                if (entity)
                {
                    m_newComponentsContext->GetRegistry().emplace<NPCAIMovementComponent>(entity);

                }
            }
            if (ImGui::MenuItem("NPC vision Component"))
            {
                m_selectionContext.AddComponent<NPCAIVisionComponent>();
                ImGui::CloseCurrentPopup();
                Entity entity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), m_selectionContext.GetComponent<IDComponent>().ID), m_newComponentsContext.get() };
                if (entity)
                {
                    m_newComponentsContext->GetRegistry().emplace<NPCAIVisionComponent>(entity);

                }
            }
            ImGui::EndPopup();
        }
        ImGui::PopItemWidth();

        DrawComponent<HealthComponent>("Health", entity, m_newComponentsContext.get(), [this, &entity](auto& component)
            {
                ImGui::DragFloat("Health", &component.Current, 0.1f, 0.0f, 100.0f);


                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), entity.GetComponent<IDComponent>().ID),
                 m_newComponentsContext.get()
                };

                if (newEntity)
                {
                    if (!newEntity.HasComponent<HealthComponent>())
                    {
                        newEntity.AddComponent<HealthComponent>();
                    }

                    newEntity.GetComponent<HealthComponent>() = component;
                }


            });

        DrawComponent<TransformComponent>("Transform", entity, m_newComponentsContext.get(), [this, &entity](auto& component)
            {
                DrawVec3Control("Translation", component.Translation, m_guizmoType, ImGuizmo::OPERATION::TRANSLATE);
                 
                glm::vec3 rotation = glm::degrees(component.Rotation);
                DrawVec3Control("Rotation", rotation, m_guizmoType, ImGuizmo::OPERATION::ROTATE);

                component.Rotation = glm::radians(rotation);
                DrawVec3Control("Scale", component.Scale, m_guizmoType, ImGuizmo::OPERATION::SCALE);

                // The saga continues here. When I modify the components, I modify them in the game registy, but also 
                // I want to save the changes to the scene - newCompnentsContext
                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_newComponentsContext.get() };
                if (newEntity)
                {
                    m_newComponentsContext->GetRegistry().get<TransformComponent>(newEntity) = component;

                }

            });

        DrawComponent<CameraComponent>("Camera", entity, m_newComponentsContext.get(), [this, &entity](auto& component)
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
                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_newComponentsContext.get() };
                if (newEntity)
                {
                    m_newComponentsContext->GetRegistry().get<CameraComponent>(newEntity) = component;
                }
            });

        DrawComponent<SpriteRendererComponent>("Sprite renderer", entity, m_newComponentsContext.get(), [this, &entity](auto& component)
            {
                ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));

                ImGui::Button("Texture", ImVec2(100.0f, 0.0f));

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                    {
                        const wchar_t* path = (const wchar_t*)payload->Data;
                        std::filesystem::path texturePath = std::filesystem::path(AssetManager::GetAssetFolderPath()) / path;

                        component.Texture = AssetManager::AddTexture(texturePath.string(), texturePath.string());
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::DragFloat("Tiling", &component.Tiling, 0.1f, 0.0f, 100.0f);
                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_newComponentsContext.get() };
                if (newEntity)
                {
                    m_newComponentsContext->GetRegistry().get<SpriteRendererComponent>(newEntity) = component;
                }
            });

        DrawComponent<CircleRendererComponent>("Circle  renderer", entity, m_newComponentsContext.get(), [this, &entity](auto& component)
            {
                ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
                ImGui::DragFloat("Fade", &component.Fade, 0.00025f, 0.0f, 1.0f);
                ImGui::DragFloat("Thickness", &component.Thickness, 0.025f, 0.0f, 1.0f);
               
                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_newComponentsContext.get() };
                if (newEntity)
                {
                    m_newComponentsContext->GetRegistry().get<CircleRendererComponent>(newEntity) = component;
                }
            });

        DrawComponent<CircleCollider2DComponent>("Circle  Collider", entity, m_newComponentsContext.get(), [this, &entity](auto& component)
            {
                ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset));
                ImGui::DragFloat("Radius", &component.Radius, 0.1f, 0.0f, 100.0f);
                ImGui::DragFloat("Density", &component.Density, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("Friction", &component.Friction, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("Restitution", &component.Restitution, 0.01, 0.0f, 1.0f);
                ImGui::DragFloat("Restitution Threshold", &component.RestitutionThershold, 0.01f, 0.0f, 1.0f);

                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_newComponentsContext.get() };
                if (newEntity)
                {
                    m_newComponentsContext->GetRegistry().get<CircleCollider2DComponent>(newEntity) = component;
                }
            });


        DrawComponent<RigidBody2DComponent>("Rigid Body2d", entity, m_newComponentsContext.get(), [this, &entity](auto& component)
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

                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_newComponentsContext.get() };
                if (newEntity)
                {
                    m_newComponentsContext->GetRegistry().get<RigidBody2DComponent>(newEntity) = component;
                }
            });
      

        DrawComponent<BoxCollider2DComponent>("Box Collider 2D", entity, m_newComponentsContext.get(), [this, &entity](auto& component)
            {
                
                ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset));
                ImGui::DragFloat2("Size", glm::value_ptr(component.Size));
                ImGui::DragFloat("Density", &component.Density, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("Friction", &component.Friction, 0.01, 0.0f, 1.0f);
                ImGui::DragFloat("Restitution", &component.Restitution, 0.01, 0.0f, 1.0f);
                ImGui::DragFloat("Restitution Threshold", &component.RestitutionThershold, 0.01f, 0.0f, 1.0f);

                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_newComponentsContext.get() };
                if (newEntity)
                {
                    m_newComponentsContext->GetRegistry().get<BoxCollider2DComponent>(newEntity) = component;
                }

            });
        DrawComponent<NPCAIMovementComponent>("NPC movement", entity, m_newComponentsContext.get(), [this, &entity](auto& component)
            {

                ImGui::DragFloat("Speed", &component.MoveSpeed, 0.01, 0.0f, 1.0f);
                ImGui::DragFloat("IdleTimer", &component.IdleTimer, 0.01, 0.0f, 1.0f);
                ImGui::DragFloat3("Target Position", glm::value_ptr(component.TargetPosition));

                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_newComponentsContext.get() };
                if (newEntity)
                {
                    m_newComponentsContext->GetRegistry().get<NPCAIMovementComponent>(newEntity) = component;
                }

            });

        DrawComponent<NPCAIVisionComponent>("NPC vision", entity, m_newComponentsContext.get(), [this, &entity](auto& component)
            {

                ImGui::DragFloat("View Radius", &component.ViewRadius, 0.01, 0.0f, 1.0f);
                ImGui::DragFloat("View Angle", &component.ViewAngle, 0.01, 0.0f, 1.0f);

                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_newComponentsContext.get() };
                if (newEntity)
                {
                    m_newComponentsContext->GetRegistry().get<NPCAIVisionComponent>(newEntity) = component;
                }

            });

        DrawComponent<ProjectileComponent>("Projectile", entity, m_newComponentsContext.get(), [this, &entity](auto& component)
            {
                ImGui::DragFloat2("Velocity", glm::value_ptr(component.Velocity));
                ImGui::DragFloat("Life time", &component.LifeTime, 0.01, 0.0f, 1.0f);
                ImGui::DragFloat("Damage", &component.Damage, 0.01, 0.0f, 1.0f);

                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_newComponentsContext.get() };
                if (newEntity)
                {
                    m_newComponentsContext->GetRegistry().get<ProjectileComponent>(newEntity) = component;
                }

            });

        // Add checks for other components here...
    }



}

