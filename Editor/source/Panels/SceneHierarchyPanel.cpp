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
#include <Engine/Scene/Components/Combat/WeaponComponent.h>
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>
#include <Engine/Scene/Components/Render/TileComponent.h>
#include <Engine/Scene/Components/Vehicles/VehicleComponent.h>
#include <Engine/Scene/Components/Projectiles/ProjectileComponent.h>
#include "Utils/EditorUtils.h"
#include <Engine/Scene/Components/Render/3D/AnimatorComponent.h>
#include <Engine/Scene/Components/Spawning/NpcSpawnControllerComponent.h>
#include <Engine/Scene/Components/Player/PlayerVisionComponent.h>
#include <Engine/Scene/Components/Light/SpotLightComponent.h>
#include <Engine/Scene/Components/Light/PointLightComponent.h>
#include <Engine/Scene/Components/Light/DirectionalLightComponent.h>


namespace Engine {

    extern const std::filesystem::path s_assetPath;


    SceneHierarchyPanel::SceneHierarchyPanel()
    {
		m_sceneHierarchyPanelScene = std::make_shared<Scene>();
    }

    

    void SceneHierarchyPanel::SetSceneHierarchyPanelScene(const Ref<Scene>& context)
    {
        m_sceneHierarchyPanelScene = context;
        m_selectedEntity = {};
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
        m_selectedEntity = entity;

    }

    void SceneHierarchyPanel::DestrtoySelectedEntity(Entity entity)
    {
       
        m_sceneHierarchyPanelScene->DestroyEntity(entity);

        if (m_selectedEntity == entity)
        {
            m_selectedEntity = {};
        }
    }

    void SceneHierarchyPanel::DrawEntityNode(Entity entity)
    {
        auto& tagComp = entity.GetComponent<TagComponent>();

        ImGuiTreeNodeFlags flags = (m_selectedEntity == entity ? ImGuiTreeNodeFlags_Selected : 0)
            | ImGuiTreeNodeFlags_SpanAvailWidth;

        bool hasExpandableContent = entity.HasComponent<TileComponent>(); // add other checks if needed
        if (!hasExpandableContent)
            flags |= ImGuiTreeNodeFlags_Leaf;
        else
            flags |= ImGuiTreeNodeFlags_OpenOnArrow;

        bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tagComp.Tag.c_str());
        m_itemIsClicked = false;

        if (ImGui::IsItemClicked())
        {
            m_selectedEntity = entity;
            m_itemIsClicked = true;
        }

        bool entityDeleted = false;

        if (ImGui::BeginPopupContextItem())
        {
            m_itemIsClicked = true;
            if (ImGui::MenuItem("Delete entity"))
                entityDeleted = true;
            ImGui::EndPopup();
        }

        if (opened)
        {
            // Draw TileComponent tiles if present
            if (entity.HasComponent<TileComponent>())
            {
                auto& tileComp = entity.GetComponent<TileComponent>();

                if (ImGui::TreeNodeEx("TileComponent", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
                {
                    for (size_t i = 0; i < tileComp.tiles.size(); i++)
                    {
                        const auto& tile = tileComp.tiles[i];
                        std::string label = tile.name + " (" + std::to_string(tile.position.x) + ", " + std::to_string(tile.position.y) + ")";

                        ImGui::PushID((int)i);

                        if (ImGui::SmallButton("X"))
                        {
                            // Erase the selected tile
                            tileComp.tiles.erase(tileComp.tiles.begin() + i);

                            // Adjust selection index if needed
                            if (m_selectedTileIndex >= tileComp.tiles.size())
                                m_selectedTileIndex = tileComp.tiles.empty() ? -1 : tileComp.tiles.size() - 1;

                            ImGui::PopID();
                            break; // Break to avoid using invalidated iterator after erase
                        }

                        ImGui::SameLine();

                        if (ImGui::Selectable(label.c_str(), m_selectedTileIndex == i))
                        {
                            SetSelectedTileIndex(i);
                        }

                       

                        ImGui::PopID();
                    }

                    ImGui::TreePop();
                }
            }

            ImGui::TreePop();
        }


        if (entityDeleted)
        {
            DestrtoySelectedEntity(entity); // typo: should be DestroySelectedEntity
        }
    }


    void SceneHierarchyPanel::DrawContext()
    {
        EE_PROFILE_FUNCTION();

        ImGui::Begin("Scene Hierarchy");

        bool clickedOnEmptySpace = true;  // Track if no entity was clicked

 
        ImGui::PushID((void*)m_sceneHierarchyPanelScene.get()); // Push scene ID to make entity IDs unique

        auto view = m_sceneHierarchyPanelScene->m_registry.view<TagComponent>();
        std::vector<entt::entity> entityList(view.begin(), view.end());
        std::reverse(entityList.begin(), entityList.end());

        ImGui::PushID((void*)m_sceneHierarchyPanelScene.get());

        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(entityList.size()));

        m_entityCount = (int)entityList.size();
        m_projectileCount = 0;
		m_tileCount = 0;
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
            {
                entt::entity entityID = entityList[i];
                Entity entity{ entityID, m_sceneHierarchyPanelScene.get() };

                ImGui::PushID((uint32_t)entityID);
                DrawEntityNode(entity);
                ImGui::PopID();

                if (entity.HasComponent<ProjectileComponent>())
                {
                    m_projectileCount++;
                }

                if (entity.HasComponent<TileComponent>())
                {
                    TileComponent& tileComp = entity.GetComponent<TileComponent>();
					m_tileCount += tileComp.tiles.size();

                }
            }
        }

        ImGui::PopID();
        ImGui::PopID();
        
       
        

        
        // Open "Create Empty Entity" popup **only** if no entity was clicked
        
        if (!m_itemIsClicked && !m_selectedEntity && ImGui::BeginPopupContextWindow("CreateEntityPopup", ImGuiPopupFlags_MouseButtonRight))
        {
            if (ImGui::MenuItem("Create Empty Entity"))
            {
                // add new component to both of the registries. m_gameContext for normal rendering and interaciton
                // m_newComponentsContext is used to save the scene and avoid any of the entities created in game scene 
                // through code to be inclued in the saved scene file
                Entity newEntity = m_sceneHierarchyPanelScene->CreateEntity("Empty entity");
               // m_gameContext->CreateEntityWithUUID(newEntity.GetUUID(), "empty Entity");

            }
            ImGui::EndPopup();
        }
        

        // Reset selection if clicking on empty space
        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && clickedOnEmptySpace)
        {
            m_selectedEntity = {};
        }

        ImGui::End();

        // Properties panel
        ImGui::Begin("Properties");
        if (m_selectedEntity)
        {
            DrawComponents(m_selectedEntity);
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
                m_sceneHierarchyPanelScene->GetRegistry().get<TagComponent>(entity).Tag = tag;
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


            if (!m_selectedEntity.HasComponent<CameraComponent>())
            {
                if (ImGui::MenuItem("Camera"))
                {
                    m_selectedEntity.AddComponent<CameraComponent>();
                    ImGui::CloseCurrentPopup();

                    // I add new components for the game registry. I also want to add it to newcomponetsContext, which
                    // will be used for saving scene. Here I look for the corresponding entity - with UUID - from another scene
                    // add add component to it 
                    Entity entity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), m_selectedEntity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                    if (entity)
                    {
                        m_sceneHierarchyPanelScene->GetRegistry().emplace<CameraComponent>(entity);
                    }
                }
            }
            if (!m_selectedEntity.HasComponent<SpriteRendererComponent>())
            {
                if (ImGui::MenuItem("Sprite renderer"))
                {
                    ImGui::CloseCurrentPopup();
                    Entity entity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), m_selectedEntity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                    if (entity)
                    {
                        SpriteRendererComponent& spriteComp = entity.AddComponent<SpriteRendererComponent>();
                        spriteComp.Texture = AssetManager::CloneTexture("logo");

                        EE_CORE_INFO("new texture");
                    }
                }
            }
            if (!m_selectedEntity.HasComponent<TileComponent>())
            {
                if (ImGui::MenuItem("Tile "))
                {
                    m_selectedEntity.AddComponent<TileComponent>();
                    ImGui::CloseCurrentPopup();
                    Entity entity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), m_selectedEntity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                    if (entity)
                    {
                        TileComponent& tileComp = entity.AddComponent<TileComponent>();
                        tileComp.Texture = AssetManager::CloneTexture("logo");

                        EE_CORE_INFO("new tile");
                    }
                }
            }
            if (!m_selectedEntity.HasComponent<CircleRendererComponent>())
            {
                if (ImGui::MenuItem("Circle renderer"))
                {
                    m_selectedEntity.AddComponent<CircleRendererComponent>();
                    ImGui::CloseCurrentPopup();
                    Entity entity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), m_selectedEntity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                    if (entity)
                    {
                        m_sceneHierarchyPanelScene->GetRegistry().emplace<CircleRendererComponent>(entity);

                    }
                }
            }
            if (!m_selectedEntity.HasComponent<TransformComponent>())
            {
                if (ImGui::MenuItem("Transform"))
                {
                    ImGui::CloseCurrentPopup();
                    Entity entity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), m_selectedEntity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get()};
                    if (entity)
                    {
                        entity.AddComponent<TransformComponent>();
                    }

                }

            }
            if (!m_selectedEntity.HasComponent<RigidBody2DComponent>())
            {
                if (ImGui::MenuItem("RigidBody 2D"))
                {
                    m_selectedEntity.AddComponent<RigidBody2DComponent>();
                    ImGui::CloseCurrentPopup();
                    Entity entity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), m_selectedEntity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                    if (entity)
                    {
                        m_sceneHierarchyPanelScene->GetRegistry().emplace<RigidBody2DComponent>(entity);

                    }
                }
            }

            if (ImGui::MenuItem("BoxCollider 2D"))
            {
                m_selectedEntity.AddComponent<BoxCollider2DComponent>();
                ImGui::CloseCurrentPopup();
                Entity entity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), m_selectedEntity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                if (entity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry().emplace<BoxCollider2DComponent>(entity);

                }
            }

            if (ImGui::MenuItem("Circle Collider"))
            {
                ImGui::CloseCurrentPopup();
                Entity entity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), m_selectedEntity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                if (entity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry().emplace<CircleCollider2DComponent>(entity);

                }
            }
            if (ImGui::MenuItem("Health Component"))
            {
                ImGui::CloseCurrentPopup();
                Entity entity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), m_selectedEntity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                if (entity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry().emplace<HealthComponent>(entity);

                }
            }
            if (ImGui::MenuItem("Weapon Component"))
            {
                ImGui::CloseCurrentPopup();
                Entity entity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), m_selectedEntity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                if (entity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry().emplace<WeaponComponent>(entity);

                }
            }
            if (ImGui::MenuItem("NPC movement Component"))
            {
                ImGui::CloseCurrentPopup();
                Entity entity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), m_selectedEntity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                if (entity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry().emplace<NPCAIMovementComponent>(entity);

                }
            }
            if (ImGui::MenuItem("NPC vision Component"))
            {
                ImGui::CloseCurrentPopup();
                Entity entity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), m_selectedEntity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                if (entity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry().emplace<NPCAIVisionComponent>(entity);

                }
            }
            if (ImGui::MenuItem("Character Controller Component"))
            {
                m_selectedEntity.AddComponent<CharacterControllerComponent>();
                ImGui::CloseCurrentPopup();
                Entity entity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), m_selectedEntity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                if (entity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry().emplace<CharacterControllerComponent>(entity);

                }
            }
            if (ImGui::MenuItem("Tile Component"))
            {
                ImGui::CloseCurrentPopup();
                Entity entity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), m_selectedEntity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                if (entity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry().emplace<TileComponent>(entity);

                }
            }
            if (ImGui::MenuItem("Vehicle Component"))
            {
                ImGui::CloseCurrentPopup();
                Entity entity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), m_selectedEntity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                if (entity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry().emplace<VehicleComponent>(entity);
                }
            }

            ImGui::EndPopup();
        }
        ImGui::PopItemWidth();

        DrawComponent<CharacterControllerComponent>("Character controller", entity, m_sceneHierarchyPanelScene.get(), [this, &entity](auto& component)
            {
                ImGui::DragFloat("speed", &component.speed, 0.1f, 0.0f, 100.0f);
                ImGui::DragFloat2("velocity", glm::value_ptr(component.velocity));

                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), entity.GetComponent<IDComponent>().ID),
                 m_sceneHierarchyPanelScene.get()
                };

                if (newEntity)
                {
                    if (!newEntity.HasComponent<CharacterControllerComponent>())
                    {
                        newEntity.AddComponent<CharacterControllerComponent>();
                    }

                    newEntity.GetComponent<CharacterControllerComponent>() = component;
                }


            });
        DrawComponent<WeaponComponent>("Weapon", entity, m_sceneHierarchyPanelScene.get(),
            [this, &entity](auto& component)
            {
                WeaponComponent& w = component;

                // 1) Weapon type combo + detect change
                {
                    static const char* weaponTypeNames[] = {
                        "Melee",
                        "Pistol",
                        "Shotgun",
                        "MachineGun",
                        "Grenade",
                        "Bazooka"
                    };

                    WeaponType prevType = w.type;
                    int typeIndex = static_cast<int>(w.type);

                    if (ImGui::Combo("Type", &typeIndex, weaponTypeNames, IM_ARRAYSIZE(weaponTypeNames)))
                    {
                        w.type = static_cast<WeaponType>(typeIndex);

                        // If type actually changed: apply default preset
                        if (w.type != prevType)
                        {
                            switch (w.type)
                            {
                            case WeaponType::Melee:
                                w.Damage = 20;
                                w.FireRate = 0.5f;
                                w.ProjectileSpeed = 0.0f;  // no projectile
                                w.MaxRange = 0.0f;
                                w.MeleeRange = 1.5f;
                                w.MeleeArcDegrees = 90.0f;
                                w.Pellets = 1;
                                w.SpreadDegrees = 0.0f;
                                w.Automatic = false;
                                w.Explosive = false;
                                w.ExplosionRadius = 0.0f;
                                w.DestructionRadius = 0.0f;
                                break;

                            case WeaponType::Pistol:
                                w.Damage = 10;
                                w.FireRate = 0.35f;
                                w.ProjectileSpeed = 25.0f;
                                w.MaxRange = 30.0f;
                                w.SpreadDegrees = 2.0f;
                                w.Pellets = 1;
                                w.Automatic = false;
                                w.Explosive = false;
                                w.ExplosionRadius = 0.0f;
                                w.DestructionRadius = 0.1f;
                                break;

                            case WeaponType::Shotgun:
                                w.Damage = 4;
                                w.FireRate = 0.8f;
                                w.ProjectileSpeed = 20.0f;
                                w.MaxRange = 15.0f;
                                w.Pellets = 8;
                                w.SpreadDegrees = 10.0f;
                                w.Automatic = false;
                                w.Explosive = false;
                                w.ExplosionRadius = 0.0f;
                                w.DestructionRadius = 0.1f;
                                break;

                            case WeaponType::MachineGun:
                                w.Damage = 6;
                                w.FireRate = 0.1f;
                                w.ProjectileSpeed = 28.0f;
                                w.MaxRange = 35.0f;
                                w.SpreadDegrees = 5.0f;
                                w.Pellets = 1;
                                w.Automatic = true;
                                w.Explosive = false;
                                w.ExplosionRadius = 0.0f;
                                w.DestructionRadius = 0.1f;
                                break;

                            case WeaponType::Grenade:
                                w.Damage = 40;
                                w.FireRate = 1.2f;
                                w.ProjectileSpeed = 12.0f;
                                w.MaxRange = 20.0f;
                                w.SpreadDegrees = 0.0f;
                                w.Pellets = 1;
                                w.Automatic = false;
                                w.Explosive = true;
                                w.ExplosionRadius = 4.0f;
                                w.DestructionRadius = 2.0f;
                                break;

                            case WeaponType::Bazooka:
                                w.Damage = 80;
                                w.FireRate = 1.5f;
                                w.ProjectileSpeed = 18.0f;
                                w.MaxRange = 40.0f;
                                w.SpreadDegrees = 1.5f;
                                w.Pellets = 1;
                                w.Automatic = false;
                                w.Explosive = true;
                                w.ExplosionRadius = 6.0f;
                                w.DestructionRadius = 3.0f;
                                break;
                            }

                            // Reset runtime stuff
                            w.Cooldown = 0.0f;
                        }
                    }
                }

                // 2) The rest of your controls (same as before, just using w)

                {
                    const uint32_t minD = 0;
                    const uint32_t maxD = 256;
                    ImGui::DragScalar("Damage", ImGuiDataType_U32, &w.Damage,
                        1.0f, &minD, &maxD, "%u");
                }

                ImGui::DragFloat("Fire Rate", &w.FireRate, 0.01f, 0.01f, 10.0f);

                ImGui::BeginDisabled();
                ImGui::DragFloat("Cooldown", &w.Cooldown, 0.01f, 0.0f, 100.0f);
                ImGui::EndDisabled();

                ImGui::DragFloat("Projectile Speed", &w.ProjectileSpeed, 0.1f, 0.0f, 200.0f);
                ImGui::DragFloat("Max Range", &w.MaxRange, 0.1f, 0.0f, 500.0f);

                ImGui::DragFloat("Destruction Radius (world)", &w.DestructionRadius,
                    0.01f, 0.0f, 100.0f, "%.3f");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Radius in world units");

                if (w.type == WeaponType::Pistol ||
                    w.type == WeaponType::MachineGun ||
                    w.type == WeaponType::Shotgun ||
                    w.type == WeaponType::Bazooka)
                {
                    ImGui::DragFloat("Spread (deg)", &w.SpreadDegrees, 0.1f, 0.0f, 45.0f);
                }

                if (w.type == WeaponType::Shotgun)
                {
                    int pellets = static_cast<int>(w.Pellets);
                    if (ImGui::DragInt("Pellets", &pellets, 1.0f, 1, 64))
                        w.Pellets = static_cast<uint32_t>(pellets);
                }

                if (w.type == WeaponType::MachineGun)
                {
                    ImGui::Checkbox("Automatic", &w.Automatic);
                }

                if (w.type == WeaponType::Melee)
                {
                    ImGui::DragFloat("Melee Range", &w.MeleeRange, 0.01f, 0.0f, 10.0f);
                    ImGui::DragFloat("Melee Arc (deg)", &w.MeleeArcDegrees, 1.0f, 0.0f, 360.0f);
                }

                if (w.type == WeaponType::Grenade || w.type == WeaponType::Bazooka)
                {
                    w.Explosive = true;
                    ImGui::DragFloat("Explosion Radius", &w.ExplosionRadius, 0.1f, 0.0f, 50.0f);
                }
            });




            
        DrawComponent<VehicleComponent>("Vehicle", entity, m_sceneHierarchyPanelScene.get(), [this, &entity](auto& component)
            {
                ImGui::DragFloat("Mass", &component.Mass, 0.1f, 0.0f, 100.0f);
                ImGui::DragFloat2("Velocity", glm::value_ptr(component.Velocity));


                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), entity.GetComponent<IDComponent>().ID),
                 m_sceneHierarchyPanelScene.get()
                };

                if (newEntity)
                {
                    if (!newEntity.HasComponent<VehicleComponent>())
                    {
                        newEntity.AddComponent<VehicleComponent>();
                    }

                    newEntity.GetComponent<VehicleComponent>() = component;
                }


            });
        DrawComponent<HealthComponent>("Health", entity, m_sceneHierarchyPanelScene.get(), [this, &entity](auto& component)
            {
                ImGui::DragFloat("Health", &component.Current, 0.1f, 0.0f, 100.0f);


                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), entity.GetComponent<IDComponent>().ID),
                 m_sceneHierarchyPanelScene.get()
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

        DrawComponent<TransformComponent>("Transform", entity, m_sceneHierarchyPanelScene.get(), [this, &entity](auto& component)
            {
                DrawVec3Control("Translation", component.Translation, m_guizmoType, ImGuizmo::OPERATION::TRANSLATE);
                 
                glm::vec3 rotation = glm::degrees(component.Rotation);
                DrawVec3Control("Rotation", rotation, m_guizmoType, ImGuizmo::OPERATION::ROTATE);

                component.Rotation = glm::radians(rotation);
                DrawVec3Control("Scale", component.Scale, m_guizmoType, ImGuizmo::OPERATION::SCALE);

                // The saga continues here. When I modify the components, I modify them in the game registy, but also 
                // I want to save the changes to the scene - newCompnentsContext 
                // wtf is this?
                //Entity newEntity = Entity{ Scene::GetEntityByUUID(m_newComponentsContext->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_newComponentsContext.get() };
                //if (newEntity)
                {
                  //  m_newComponentsContext->GetRegistry().get<TransformComponent>(newEntity) = component;

                }

            });

        DrawComponent<CameraComponent>("Camera", entity, m_sceneHierarchyPanelScene.get(), [this, &entity](auto& component)
            {
                auto& cameraComp = component;
                auto& camera = component.Camera;

                ImGui::Checkbox("Primary", &cameraComp.Primary);
                ImGui::Checkbox("Free camera", &cameraComp.FreeCamera);

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
                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                if (newEntity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry().get<CameraComponent>(newEntity) = component;
                }
            });

        DrawComponent<SpriteRendererComponent>("Sprite renderer", entity, m_sceneHierarchyPanelScene.get(), [this, &entity](auto& component)
            {
                ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));

                // Label
                ImGui::Text("Texture");
                ImGui::SameLine();

                // Button with texture name or "Add"
                const char* textureButtonText = component.Texture
                    ? component.Texture->GetName().c_str()
                    : "Add";

                ImGui::Button(textureButtonText, ImVec2(150.0f, 0.0f)); // Wider to fit longer names

                // Drag & Drop Handling
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                    {
                        const wchar_t* path = (const wchar_t*)payload->Data;
                        std::filesystem::path texturePath = std::filesystem::path(AssetManager::GetAssetFolderPath()) / path;
                        std::string textureName = texturePath.stem().string();

                        component.Texture = AssetManager::GetTexture(textureName);
                        if (!component.Texture)
                        {
                            component.Texture = AssetManager::AddTexture(textureName, texturePath.string());
                        }
                        if (!component.Texture)
                        {
                            component.Texture = AssetManager::GetTexture("logo");
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::DragFloat("Tiling", &component.Tiling, 0.1f, 0.0f, 100.0f);

                // Update entity in new context
                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                if (newEntity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry().get<SpriteRendererComponent>(newEntity) = component;
                }

            });

        DrawComponent<TileComponent>("Tile", entity, m_sceneHierarchyPanelScene.get(), [this, &entity](auto& component)
            {
                ImGui::Text("Grid Pos");
                ImGui::SeparatorText("Texture");

                const char* textureButtonText = component.Texture
                    ? component.Texture->GetName().c_str()
                    : "Add";

                if (ImGui::Button(textureButtonText, ImVec2(150.0f, 0.0f)))
                {
                    // Optional: open file dialog or asset browser popup
                }

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                    {
                        const wchar_t* path = (const wchar_t*)payload->Data;
                        std::filesystem::path texturePath = std::filesystem::path(AssetManager::GetAssetFolderPath()) / path;
                        std::string textureName = texturePath.stem().string();

                        component.Texture = AssetManager::GetTexture(textureName);
                        if (!component.Texture)
                            component.Texture = AssetManager::AddTexture(textureName, texturePath.string());
                        if (!component.Texture)
                            component.Texture = AssetManager::GetTexture("logo"); // fallback
                    }
                    ImGui::EndDragDropTarget();
                }


                std::optional<size_t> previousSelected = m_previousSelected;

                // Ensure selected tile is open
                if (m_selectedTileIndex.has_value())
                {
                    m_openTileIndices.insert(*m_selectedTileIndex);
                }

                // Set scroll flag only if selection changed externally
                if (m_selectedTileIndex && (!previousSelected || *previousSelected != *m_selectedTileIndex))
                {
                    m_openTileIndices.insert(*m_selectedTileIndex);
                    m_scrollToSelectedTileNextFrame = true;
                }

                for (size_t i = 0; i < component.tiles.size(); ++i)
                {
                    TileInfo& tile = component.tiles[i];
                    bool isOpen = m_openTileIndices.find(i) != m_openTileIndices.end();

                    std::string label = tile.name + " (" + std::to_string(tile.position.x) + ", " + std::to_string(tile.position.y) + ")";

                    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_SpanAvailWidth;
                    bool isSelected = m_selectedTileIndex && *m_selectedTileIndex == i;
                    if (isOpen)
                        nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
                    if (isSelected)
                        nodeFlags |= ImGuiTreeNodeFlags_Selected;

                    if (isSelected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.26f, 0.39f, 0.98f, 0.9f));

                    bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)i, nodeFlags, "%s", label.c_str());

                    // Handle UI clicks to select tile
                    if (ImGui::IsItemClicked())
                    {
                        if (!m_selectedTileIndex || *m_selectedTileIndex != i)
                        {
                            m_selectedTileIndex = i;
                            m_openTileIndices.insert(i);
                            m_scrollToSelectedTileNextFrame = true;  // Set scroll flag on UI selection
                        }
                    }

                    if (isSelected)
                    {
                        if (m_scrollToSelectedTileNextFrame)
                        {
                            ImGui::SetScrollHereY(0.25f);
                            m_scrollToSelectedTileNextFrame = false;  // Reset scroll flag here so it scrolls once
                        }
                        ImGui::PopStyleColor();
                    }

                    if (nodeOpen)
                    {
                        ImGui::Indent();

                        ImGui::Text("Position: %.2f, %.2f", tile.position.x, tile.position.y);
                        ImGui::Checkbox("Destructible", &tile.IsDestructible);
                        ImGui::Checkbox("Roof", &tile.IsRoof);

                        static char nameBuffer[256];
                        strncpy(nameBuffer, tile.name.c_str(), sizeof(nameBuffer));
                        nameBuffer[sizeof(nameBuffer) - 1] = '\0';
                        if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
                            tile.name = std::string(nameBuffer);

                        static const char* materialOptions[] = { "None", "Wood", "Concrete", "Metal", "Glass" };
                        int currentMaterialIdx = static_cast<int>(tile.Material);
                        if (ImGui::Combo("Material", &currentMaterialIdx, materialOptions, IM_ARRAYSIZE(materialOptions)))
                            tile.Material = static_cast<eTileMaterial>(currentMaterialIdx);

                        ImGui::InputScalar("Health", ImGuiDataType_U32, &tile.TileHealth);

                        ImGui::Unindent();
                        ImGui::TreePop();
                    }
                    else
                    {
                        m_openTileIndices.erase(i);
                    }
                }

                // Update previous selected tile for next frame
                m_previousSelected = m_selectedTileIndex;


                // Sync the component back (optional depending on ECS design)
                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                if (newEntity)
                    m_sceneHierarchyPanelScene->GetRegistry().get<TileComponent>(newEntity) = component;
            });



        DrawComponent<CircleRendererComponent>("Circle  renderer", entity, m_sceneHierarchyPanelScene.get(), [this, &entity](auto& component)
            {
                ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
                ImGui::DragFloat("Fade", &component.Fade, 0.00025f, 0.0f, 1.0f);
                ImGui::DragFloat("Thickness", &component.Thickness, 0.025f, 0.0f, 1.0f);
               
                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                if (newEntity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry().get<CircleRendererComponent>(newEntity) = component;
                }
            });

        DrawComponent<CircleCollider2DComponent>("Circle  Collider", entity, m_sceneHierarchyPanelScene.get(), [this, &entity](auto& component)
            {
                ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset));
                ImGui::DragFloat("Radius", &component.Radius, 0.1f, 0.0f, 100.0f);
                ImGui::DragFloat("Density", &component.Density, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("Friction", &component.Friction, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("Restitution", &component.Restitution, 0.01, 0.0f, 1.0f);
                ImGui::DragFloat("Restitution Threshold", &component.RestitutionThershold, 0.01f, 0.0f, 1.0f);

                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                if (newEntity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry().get<CircleCollider2DComponent>(newEntity) = component;
                }
            });


        DrawComponent<RigidBody2DComponent>("Rigid Body2d", entity, m_sceneHierarchyPanelScene.get(), [this, &entity](auto& component)
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

                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                if (newEntity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry().get<RigidBody2DComponent>(newEntity) = component;
                }
            });
      

        DrawComponent<BoxCollider2DComponent>("Box Collider 2D", entity, m_sceneHierarchyPanelScene.get(), [this, &entity](auto& component)
            {
                
                ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset));
                ImGui::DragFloat2("Size", glm::value_ptr(component.Size));
                ImGui::DragFloat("Density", &component.Density, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("Friction", &component.Friction, 0.01, 0.0f, 1.0f);
                ImGui::DragFloat("Restitution", &component.Restitution, 0.01, 0.0f, 1.0f);
                ImGui::DragFloat("Restitution Threshold", &component.RestitutionThershold, 0.01f, 0.0f, 1.0f);

                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                if (newEntity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry().get<BoxCollider2DComponent>(newEntity) = component;
                }

            });
        
        DrawComponent<DirectionalLightComponent>("Directional Light", entity, m_sceneHierarchyPanelScene.get(), [this, &entity](auto& component)
            {
                

                ImGui::DragFloat3("Direction", glm::value_ptr(component.directionWS), 0.01f);
                ImGui::ColorEdit3("Color", glm::value_ptr(component.color));
                ImGui::DragFloat("Intensity", &component.intensity, 0.01f, 0.0f, 10.0f);

                // Normalize direction button
                if (ImGui::Button("Normalize Direction"))
                {
                    component.directionWS = glm::normalize(component.directionWS);
                    
                }

               
                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                if (newEntity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry().get<DirectionalLightComponent>(newEntity) = component;
                }
                
            });

        DrawComponent<PointLightComponent>("Point Light", entity, m_sceneHierarchyPanelScene.get(), [this, &entity](auto& component)
            {
                ImGui::ColorEdit3("Color", glm::value_ptr(component.color));
                ImGui::DragFloat("Intensity", &component.intensity, 0.01f, 0.0f, 10.0f);
                ImGui::DragFloat("Radius", &component.radius, 0.1f, 0.1f, 100.0f);

                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                if (newEntity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry().get<PointLightComponent>(newEntity) = component;
                }
            });

        DrawComponent<SpotLightComponent>("Spot Light", entity, m_sceneHierarchyPanelScene.get(), [this, &entity](auto& component)
            {
                ImGui::DragFloat3("Direction", glm::value_ptr(component.directionWS), 0.01f);
                ImGui::ColorEdit3("Color", glm::value_ptr(component.color));
                ImGui::DragFloat("Intensity", &component.intensity, 0.01f, 0.0f, 10.0f);
                ImGui::DragFloat("Range", &component.range, 0.1f, 0.1f, 100.0f);

                float innerAngleDeg = glm::degrees(component.innerAngleRad);
                float outerAngleDeg = glm::degrees(component.outerAngleRad);

                if (ImGui::DragFloat("Inner Angle", &innerAngleDeg, 0.5f, 0.0f, 90.0f))
                {
                    component.innerAngleRad = glm::radians(innerAngleDeg);
                }
                if (ImGui::DragFloat("Outer Angle", &outerAngleDeg, 0.5f, 0.0f, 90.0f))
                {
                    component.outerAngleRad = glm::radians(outerAngleDeg);
                }

                if (component.innerAngleRad > component.outerAngleRad)
                {
                    component.innerAngleRad = component.outerAngleRad;
                }

                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                if (newEntity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry().get<SpotLightComponent>(newEntity) = component;
                }
            });

        DrawComponent<NPCAIMovementComponent>("NPC movement", entity, m_sceneHierarchyPanelScene.get(),
            [this, &entity](auto& component)
            {
                // ---- Read-only-ish debug ----
                ImGui::Text("Orders (from StateSystem)");
                ImGui::Text("wantsMove: %s", component.wantsMove ? "true" : "false");
                ImGui::Text("usePath:   %s", component.usePath ? "true" : "false");

                ImGui::Separator();

                // ---- Editable tuning ----
                ImGui::DragFloat("Radius", &component.radius, 0.01f, 0.0f, 2.0f);
                ImGui::DragFloat("Speed", &component.moveSpeed, 0.01f, 0.0f, 20.0f);

                // These are technically "orders", but can be handy to inspect/edit while debugging.
                // If you want them read-only, replace with ImGui::Text.
                ImGui::Checkbox("Debug wantsMove", (bool*)&component.wantsMove);
                ImGui::Checkbox("Debug usePath", (bool*)&component.usePath);
                ImGui::DragFloat2("Debug goal2D", &component.goal2D.x, 0.01f);

                ImGui::Checkbox("Debug hasFacing", (bool*)&component.hasFacing);
                ImGui::DragFloat2("Debug facing2D", &component.facing2D.x, 0.01f);

                ImGui::Separator();

                // ---- Path runtime debug ----
                ImGui::Text("Path runtime");
                ImGui::Text("pathIndex: %u / %u", component.pathIndex, (uint32_t)component.path.size());
                ImGui::Text("repathTimer: %.2f", component.repathTimer);
                ImGui::DragFloat("repathInterval", &component.repathInterval, 0.01f, 0.01f, 5.0f);

                ImGui::DragFloat2("lastRepathStart2D", &component.lastRepathStart2D.x, 0.01f);
                ImGui::DragFloat2("lastRepathGoal2D", &component.lastRepathGoal2D.x, 0.01f);

                if (ImGui::Button("Clear Path"))
                    component.ClearPath();

                // ---- Write back to registry copy (your pattern) ----
                Entity newEntity = Entity{ Scene::GetEntityByUUID(
                    m_sceneHierarchyPanelScene->GetRegistry(),
                    entity.GetComponent<IDComponent>().ID),
                    m_sceneHierarchyPanelScene.get() };

                if (newEntity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry()
                        .get<NPCAIMovementComponent>(newEntity) = component;
                }
            });

        
        DrawComponent<NpcSpawnControllerComponent>("NPC spawner", entity, m_sceneHierarchyPanelScene.get(),
            [this, &entity](auto& component)
            {
                // ---- Enable ----
                ImGui::Checkbox("Enabled", (bool*)&component.enabled);

                ImGui::Separator();

                // ---- Limits / timing ----
                ImGui::DragInt("Max Alive", (int*)&component.maxAlive, 1.0f, 0, 10000);
                ImGui::DragFloat("Spawn Interval", &component.spawnInterval, 0.01f, 0.01f, 60.0f);
                ImGui::DragInt("Spawn Batch", (int*)&component.spawnBatch, 1.0f, 1, 1000);

                ImGui::Separator();

                // ---- Spawn radius ----
                ImGui::DragFloat("Spawn Radius Min", &component.spawnRadiusMin, 0.05f, 0.0f, 1000.0f);
                ImGui::DragFloat("Spawn Radius Max", &component.spawnRadiusMax, 0.05f, 0.0f, 1000.0f);

                if (component.spawnRadiusMax < component.spawnRadiusMin)
                    component.spawnRadiusMax = component.spawnRadiusMin;

                ImGui::Separator();

                // ---- Wave mode ----
                ImGui::Text("Wave");
                ImGui::DragInt("Total To Spawn (0 = infinite)", (int*)&component.totalToSpawn, 1.0f, 0, 1000000);
                ImGui::Text("Spawned So Far: %u", component.spawnedSoFar);

                if (ImGui::Button("Reset Spawned Counter"))
                    component.spawnedSoFar = 0;

                ImGui::Separator();

                // ---- Prefab ----
                ImGui::DragInt("npcPrefabId", (int*)&component.npcPrefabId, 1.0f, 0, 1000000);

                ImGui::Separator();

                // ---- Runtime debug ----
                ImGui::Text("Runtime");
                ImGui::Text("Alive (cached): %u", component.aliveCached);
                ImGui::Text("Tracked spawned: %u", (uint32_t)component.spawned.size());
                ImGui::Text("spawnTimer: %.2f / %.2f", component.spawnTimer, component.spawnInterval);

                ImGui::DragFloat("Prune Interval", &component.pruneInterval, 0.05f, 0.05f, 10.0f);
                ImGui::Text("pruneTimer: %.2f", component.pruneTimer);

                if (ImGui::Button("Clear Tracked List"))
                    component.spawned.clear();

                // ---- Write back to registry copy (your pattern) ----
                Entity newEntity = Entity{ Scene::GetEntityByUUID(
                    m_sceneHierarchyPanelScene->GetRegistry(),
                    entity.GetComponent<IDComponent>().ID),
                    m_sceneHierarchyPanelScene.get() };

                if (newEntity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry()
                        .get<NpcSpawnControllerComponent>(newEntity) = component;
                }
            });

        DrawComponent<NPCAIVisionComponent>("NPC vision", entity, m_sceneHierarchyPanelScene.get(),
            [this, &entity](auto& component)
            {
                // ---- Editable tuning ----
                ImGui::DragFloat("ViewRadius", &component.ViewRadius, 0.05f, 0.0f, 100.0f);
                ImGui::DragFloat("ViewAngle", &component.ViewAngle, 1.0f, 0.0f, 360.0f);

                ImGui::DragFloat("LOS Interval", &component.losCheckInterval, 0.01f, 0.01f, 1.0f);
                ImGui::Checkbox("Debug show LOS", (bool*)&component.debugDraw);


                ImGui::Separator();

                // ---- Cached debug (read-only) ----
                ImGui::Text("Cached result");
                ImGui::Text("hasLOS: %s", component.hasLOS ? "true" : "false");
                ImGui::Text("distToTarget: %.2f", component.distToTarget);
                ImGui::Text("timeSinceSeen: %.2f", component.timeSinceSeen);

                ImGui::Text("lastSeenPos: [%.2f, %.2f, %.2f]",
                    component.lastSeenPos.x, component.lastSeenPos.y, component.lastSeenPos.z);

                ImGui::Text("losCheckTimer: %.3f", component.losCheckTimer);

                // Entity debug (only print IDs if you have them; otherwise just show validity)
                ImGui::Text("VisibleTarget valid: %s", component.VisibleTarget ? "true" : "false");
                ImGui::Text("lastSeenTarget valid: %s", component.lastSeenTarget ? "true" : "false");

                
                // ---- Write back to registry copy (your pattern) ----
                Entity newEntity = Entity{ Scene::GetEntityByUUID(
                    m_sceneHierarchyPanelScene->GetRegistry(),
                    entity.GetComponent<IDComponent>().ID),
                    m_sceneHierarchyPanelScene.get() };

                if (newEntity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry()
                        .get<NPCAIVisionComponent>(newEntity) = component;
                }
            });

        DrawComponent<NpcAIStateComponent>("NPC AI state", entity, m_sceneHierarchyPanelScene.get(),
            [this, &entity](auto& component)
            {
                // ---- Read-only state ----
                ImGui::Text("State: %s", EditorUtils::AIStateToString(component.state));

                ImGui::Separator();

                // ---- Editable tuning ----
                ImGui::DragFloat("idleDuration", &component.idleDuration, 0.01f, 0.0f, 10.0f);

                // Patrol index is runtime, but editable for debugging
                ImGui::DragInt("patrolIndex", (int*)&component.patrolIndex, 1.0f, 0, 1024);

                // If you still use these in StateSystem, keep editable; otherwise remove.
                ImGui::DragFloat("repathInterval", &component.repathInterval, 0.01f, 0.01f, 5.0f);

                ImGui::Separator();

                // ---- Runtime debug (read-only-ish) ----
                ImGui::Text("Runtime");
                ImGui::Text("idleTimer: %.2f", component.idleTimer);
                ImGui::Text("repathTimer: %.2f", component.repathTimer);

                ImGui::Text("hasLastKnown: %s", component.hasLastKnown ? "true" : "false");
                ImGui::Text("lastKnownPos: [%.2f, %.2f, %.2f]",
                    component.lastKnownPos.x, component.lastKnownPos.y, component.lastKnownPos.z);

                ImGui::Separator();

                // ---- Orders (debug) ----
                ImGui::Text("Orders (to MovementSystem)");
                ImGui::Text("wantsMove: %s", component.wantsMove ? "true" : "false");
                ImGui::Text("wantsPath: %s", component.wantsPath ? "true" : "false");
                ImGui::Text("moveGoal2D: [%.2f, %.2f]", component.moveGoal2D.x, component.moveGoal2D.y);

                ImGui::Text("Orders (to Anim)");
                ImGui::Text("wantsAttack: %s", component.wantsAttack ? "true" : "false");

                // Optional debug editing (useful when testing)
                bool dbgWantsMove = component.wantsMove != 0;
                if (ImGui::Checkbox("Debug wantsMove", &dbgWantsMove))
                    component.wantsMove = dbgWantsMove ? 1 : 0;

                bool dbgWantsPath = component.wantsPath != 0;
                if (ImGui::Checkbox("Debug wantsPath", &dbgWantsPath))
                    component.wantsPath = dbgWantsPath ? 1 : 0;

                ImGui::DragFloat2("Debug moveGoal2D", &component.moveGoal2D.x, 0.01f);

                bool dbgWantsAttack = component.wantsAttack != 0;
                if (ImGui::Checkbox("Debug wantsAttack", &dbgWantsAttack))
                    component.wantsAttack = dbgWantsAttack ? 1 : 0;

                // ---- Write back to registry copy (your pattern) ----
                Entity newEntity = Entity{ Scene::GetEntityByUUID(
                    m_sceneHierarchyPanelScene->GetRegistry(),
                    entity.GetComponent<IDComponent>().ID),
                    m_sceneHierarchyPanelScene.get() };

                if (newEntity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry()
                        .get<NpcAIStateComponent>(newEntity) = component;
                }
            });

        DrawComponent<ProjectileComponent>("Projectile", entity, m_sceneHierarchyPanelScene.get(), [this, &entity](auto& component)
            {
                
                ImGui::DragFloat2("Velocity", glm::value_ptr(component.Direction));
                ImGui::DragFloat("Life time", &component.ProjectileMaxRange, 0.01, 0.0f, 1.0f);
                {
                    const uint32_t minD = 0;
                    const uint32_t maxD = 256;
                    // step = 1.0f, display as unsigned
                    ImGui::DragScalar("Damage", ImGuiDataType_U32, &component.Damage,
                        1.0f, &minD, &maxD, "%u");
                }
                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                if (newEntity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry().get<ProjectileComponent>(newEntity) = component;
                }

            });

        DrawComponent<PlayerVisionComp>("Player Vision", entity, m_sceneHierarchyPanelScene.get(), [this, &entity](auto& component)
            {

                ImGui::DragFloat("Vision distance", &component.visionDistanceW, 0.01, 0.0f, 20.0f);
                
                Entity newEntity = Entity{ Scene::GetEntityByUUID(m_sceneHierarchyPanelScene->GetRegistry(), entity.GetComponent<IDComponent>().ID), m_sceneHierarchyPanelScene.get() };
                if (newEntity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry().get<PlayerVisionComp>(newEntity) = component;
                }

            });

        DrawComponent<EnemyDestructibleComponent>("Enemy Destructible", entity, m_sceneHierarchyPanelScene.get(),
            [this, &entity](auto& /*component*/)
            {
                // You can tune these however you want
                glm::vec3 impulseDir = glm::normalize(glm::vec3(1.0f, 0.6f, 0.0f)); // mostly sideways + a bit up
                float impulseStrength = 6.0f;

                // Optional tweak controls:
                ImGui::DragFloat3("Detach impulse dir", glm::value_ptr(impulseDir), 0.01f, -1.0f, 1.0f);
                ImGui::DragFloat("Detach impulse strength", &impulseStrength, 0.1f, 0.0f, 100.0f);

                if (ImGui::Button("Detach both legs"))
                {
                    // Call the code that you KNOW works.
                    // IMPORTANT: This assumes you can access a ProjectileSystem instance.
                    // Replace GetProjectileSystem() with whatever you actually have.
                    
                        EditorUtils::DetachPiece(m_sceneHierarchyPanelScene.get(), entity,
                            Engine::EnemyPieceType::LegL_Thigh, impulseDir, impulseStrength);
                        EditorUtils::DetachPiece(m_sceneHierarchyPanelScene.get(), entity,
                            Engine::EnemyPieceType::LegL_Calf, impulseDir, impulseStrength);

                        EditorUtils::DetachPiece(m_sceneHierarchyPanelScene.get(), entity,
                            Engine::EnemyPieceType::LegR_Thigh, impulseDir, impulseStrength);
                        EditorUtils::DetachPiece(m_sceneHierarchyPanelScene.get(), entity,
                            Engine::EnemyPieceType::LegR_Calf, impulseDir, impulseStrength);
                    
                }
            });



        DrawComponent<Engine::Animator3DComponent>("Animator 3D", entity, m_sceneHierarchyPanelScene.get(),
            [this, &entity](auto& component)
            {
                
                auto& animReg = Engine::AssetManager::GetAnimationRegistry();
                const uint32_t INVALID = 0xFFFFFFFFu;

                auto ClipNameOr = [&](uint32_t id) -> const char*
                    {
                        if (id == INVALID) return "<none>";
                        const Engine::AnimationClip& c = animReg.Get(id);
                        return c.name.empty() ? "<unnamed>" : c.name.c_str();
                    };

                ImGui::Text("clipA: %s (%u)", ClipNameOr((uint32_t)component.clipA), (uint32_t)component.clipA);
                ImGui::Text("clipB: %s (%u)", ClipNameOr((uint32_t)component.clipB), (uint32_t)component.clipB);

                ImGui::Separator();

                ImGui::DragFloat("timeA", &component.timeA, 0.01f, 0.0f, 1000.0f);
                ImGui::DragFloat("timeB", &component.timeB, 0.01f, 0.0f, 1000.0f);

                ImGui::DragFloat("blend", &component.blend, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("playbackSpeed", &component.playbackSpeed, 0.01f, 0.0f, 5.0f);

                bool useRM = (component.useRootMotion != 0);
                if (ImGui::Checkbox("useRootMotion", &useRM))
                    component.useRootMotion = useRM ? 1 : 0;

            


                ImGui::Separator();

             
                ImGui::Separator();

                if (ImGui::Button("Reset Times"))
                {
                    component.timeA = 0.0f;
                    component.timeB = 0.0f;
                }
                ImGui::SameLine();

                if (ImGui::Button("Clear clipB"))
                {
                    component.clipB = INVALID;
                    component.timeB = 0.0f;
                    component.blend = 0.0f;
                }
                ImGui::SameLine();

                if (ImGui::Button("Swap A/B"))
                {
                    std::swap(component.clipA, component.clipB);
                    std::swap(component.timeA, component.timeB);
                    component.blend = 1.0f - component.blend;
                }

                // Write-back like your NPC movement panel
                Engine::Entity newEntity = Engine::Entity{ Engine::Scene::GetEntityByUUID(
                    m_sceneHierarchyPanelScene->GetRegistry(),
                    entity.GetComponent<Engine::IDComponent>().ID),
                    m_sceneHierarchyPanelScene.get() };

                if (newEntity)
                {
                    m_sceneHierarchyPanelScene->GetRegistry()
                        .get<Engine::Animator3DComponent>(newEntity) = component;
                }
            });

    }



}

