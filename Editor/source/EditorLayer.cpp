#include "pch.h"

#include "EditorLayer.h"

//#include <imgui/backends/imgui_impl_vulkan.h>


#include <Engine/Debug/Instrumentor.h>

#include "Engine/Scene/SceneSerializer.h"
#include "Engine/Utils/PlatformUtils.h"

#include "ImGuizmo.h"
#include "Engine/Math/Math.h"


#include "EditorApp.h"
#include <Engine/AssetManager/AssetManager.h>

#include "Engine/Renderer/Renderer.h"

#include <Engine/Scene/Components/Render/TileComponent.h>
#include "Panels/Utils/EditorUtils.h"

#include "Engine/Map/Utils/IsoTileUtils.h"
//debug
#include "Panels/Utils/EditorDebugUtils.h"
#include <Engine/Renderer/Renderer2D/VulkanRenderer2D.h>
#include <algorithm>
#include "Engine/Math/HashUtils.h"
#include <Engine/AssetManager/Utils/Statistics.h>
#include <Engine/Events/KeyEvent.h>
#include <Engine/Events/MouseEvent.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <Engine/Events/MouseCodes.h>

//#include "Commands/PlaceTileCommand.h"
#include <utility>
#include "Commands/PlaceTileCommand.h"
#include "Commands/PlaceCompactTileCommand.h"
#include <Engine/Scene/Components/Map/AreaComponent.h>
#include <Engine/Scene/Components/Light/DirectionalLightComponent.h>
#include <Engine/Map/Tile/TileManager.h>
#include <Engine/Map/Tile/CompactTileMap.h>
#include <Engine/Scene/Prefabs/PrefabSerializer.h>

namespace Engine {




    class Editor;

    EditorLayer::EditorLayer(Editor* editor)
        : Layer("EditorLayer"),
        m_editor(editor)
    {

    }

    void EditorLayer::OnAttach()
    {
        EE_PROFILE_FUNCTION();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = "imgui.ini";

        m_iconPlay = std::make_shared<VulkanTexture>(AssetManager::GetAssetPath("icons/play-button-arrowhead.png").string(), VK_FORMAT_R8G8B8A8_UNORM, "iconPlay", true);
        m_iconStop = std::make_shared<VulkanTexture>(AssetManager::GetAssetPath("icons/stop-button.png").string(), VK_FORMAT_R8G8B8A8_UNORM, "iconStop", true);
        m_iconPause = std::make_shared<VulkanTexture>(AssetManager::GetAssetPath("icons/video-pause-button.png").string(), VK_FORMAT_R8G8B8A8_UNORM, "iconPause", true);
        m_iconLoading = std::make_shared<VulkanTexture>(AssetManager::GetAssetPath("icons/loading.png").string(), VK_FORMAT_R8G8B8A8_UNORM, "iconLoading",  true);

        Engine::FramebufferSpecification framebufferSpecs;

        framebufferSpecs.Attachments = { FramebufferTextureFormat::RGBA8,FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
        framebufferSpecs.Height = 720;
        framebufferSpecs.Width = 1280;

        m_editorCamera = EditorCamera(55.0f, 1.78f, 0.1f, 1000.0f);

        /*
        class CameraController : public ScriptableEntity
        {
        public:
            void OnCreate()
            {
            }
            void OnDestroy()
            {
            }

            void OnUpdate(Timestep ts)
            {
                auto& transform = GetComponent<TransformComponent>().Translation;
                float speed = 5.0f;

                if (Input::IsKeyPressed(Key::A))
                {
                    transform -= speed * ts;
                }
                if (Input::IsKeyPressed(Key::D))
                {
                    transform += speed * ts;
                }
                if (Input::IsKeyPressed(Key::W))
                {
                    transform += speed * ts;
                }
                if (Input::IsKeyPressed(Key::S))
                {
                    transform -= speed * ts;
                }
            }

        };        

        */

        Engine::SceneSerializer serializer(m_sceneHierarchyPanel.GetEditorScene());
        std::string scenePath = AssetManager::GetScenePath(m_editor.get()->GetGameLayer()->GetActiveSceneName()).string();
        if (!serializer.Deserialize(scenePath))
        {
            EE_CORE_ERROR("Failed to load scene at: {}", scenePath);
        }

        //m_sceneHierarchyPanel.SetGameContext(m_editor.get()->GetGameLayer()->GetActiveGameScene());
        m_debugPanel.SetGameContext(m_editor.get()->GetGameLayer()->GetActiveGameScene());
        m_debugPanel.SetEditor(m_editor);
        m_currentScenePath = AssetManager::GetScenePath(m_editor.get()->GetGameLayer()->GetActiveSceneName());
        m_editor.get()->GetGameLayer()->SetActiveScene(m_sceneHierarchyPanel.GetEditorScene());
        m_sceneHierarchyPanel.SetGizmoType(ImGuizmo::OPERATION::TRANSLATE);
        m_effectsPanel.SetState(VulkanRenderer2D::GetEffects());
    }

    void EditorLayer::OnDetach()
    {
        EE_PROFILE_FUNCTION();
    }

    void EditorLayer::OnImGuiRender()
    {

        //Renderer::DrawFrame();
        //return;
        EE_PROFILE_FUNCTION();

        // READ THIS !!!
        // TL;DR; this demo is more complicated than what most users you would normally use.
        // If we remove all options we are showcasing, this demo would become:
        //     void ShowExampleAppDockSpace()
        //     {
        //         ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
        //     }
        // In most cases you should be able to just call DockSpaceOverViewport() and ignore all the code below!
        // In this specific demo, we are not using DockSpaceOverViewport() because:
        // - (1) we allow the host window to be floating/moveable instead of filling the viewport (when opt_fullscreen == false)
        // - (2) we allow the host window to have padding (when opt_padding == true)
        // - (3) we expose many flags and need a way to have them visible.
        // - (4) we have a local menu bar in the host window (vs. you could use BeginMainMenuBar() + DockSpaceOverViewport()
        //      in your code, but we don't here because we allow the window to be floating)

        static bool opt_fullscreen = true;
        static bool dockspaceOpen = true;
        static bool opt_padding = false;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen)
        {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }
        else
        {
            dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
        }

        // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
        // and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
        // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
        // all active windows docked into it will lose their parent and become undocked.
        // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
        // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
        if (!opt_padding)
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
        if (!opt_padding)
            ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);




        // Submit the DockSpace
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowMinSize.x = 350.0f;

        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }
        else
        {
            //ShowDockingDisabledMessage();
        }

        style.WindowMinSize.x = 32.0f;



        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                // Disabling fullscreen would allow the window to be moved to the front of other windows,
                // which we can't undo at the moment without finer window depth/z control.
                ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen);

                if (ImGui::MenuItem("New", "Ctrl+N"))
                {
                    NewScene();
                }

                if (ImGui::MenuItem("Open...", "Ctrl+O"))
                {
                    OpenScene();
                }
                if (ImGui::MenuItem("Save as...", "Ctrl+Shift+S"))
                {
                    SaveSceneAs();
                }
                if (ImGui::MenuItem("Save", "Ctrl+S"))
                {
                    SaveScene();
                }
                
                ImGui::Separator();

                if (ImGui::MenuItem("Exit"))
                {
                    //vkDeviceWaitIdle(VulkanContext::Get()->GetDeviceManager().GetDevice());
                    Engine::Application::Get().Close();
                }

                ImGui::Separator();


                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Prefabs"))
            {
                if (ImGui::MenuItem("Save As Prefab"))
                {
                    SaveAsPrefab();
                }
                if (ImGui::MenuItem("Load Prefab"))
                {
                    LoadPrefab();
                }
                ImGui::EndMenu();
            }
			m_debugPanel.OnImGuiRender();
            m_effectsPanel.OnImGuiRender();

            m_sceneHierarchyPanel.OnImGuiRender();
            m_tileEditorPanel.OnImGuiRender();
            m_contentBrowserPanel.OnImGuiRender();

            ImGui::Begin("Stats");
            Renderer2D::Statistics stats = Engine::VulkanRenderer2D::GetStats();
            ImGui::Text("Renderer2D Stats:");
            ImGui::Text("Draw Calls: %d", stats.DrawCalls);
            ImGui::Text("Quads: %d", stats.QuadCount);
            ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
            ImGui::Text("Indicies: %d", stats.GetTotalIndexCount());
            ImGui::Text("Lines: %d", stats.LineCount);

            GPUStats& statsGPU = GPUStats::Get();
         
            ImGui::Text("GPU Textures:  %.2f MB", statsGPU.GetTextures() / (1024.0f * 1024.0f));
            ImGui::Text("GPU Images:    %.2f MB", statsGPU.GetImages() / (1024.0f * 1024.0f));
            ImGui::Text("GPU Buffers:   %.2f MB", statsGPU.GetBuffers() / (1024.0f * 1024.0f));

            VkDeviceSize totalBytes = statsGPU.GetTextures() + statsGPU.GetBuffers();

            float totalMB = (float)totalBytes / (1024.0f * 1024.0f);

            ImGui::Separator();
            ImGui::Text("GPU Total tracked: %.2f MB", totalMB);
            ImGui::Text("FPS: %d", m_fpsCounter.GetFPS());


            VulkanRenderer3D::Statistics3D stats3D = Engine::VulkanRenderer3D::GetStats3D();
            ImGui::Text("Vertices 3D: %d", stats3D.GetVertexCount());
            ImGui::Text("Indicies 3D: %d", stats3D.GetIndexCount());


            ImGuiTreeNodeFlags flags =   ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_OpenOnArrow;
            flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
            flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

            bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)1, flags, "Entities: %d", m_sceneHierarchyPanel.GetEntityCount());
            if (opened)
            {
               // ImGui::Text("Total Entity count: %d", m_sceneHierarchyPanel.GetEntityCount());
                ImGui::Text("Projectile count: %d", m_sceneHierarchyPanel.GetProjectileCount());
                ImGui::Text("Tile count: %d", m_sceneHierarchyPanel.GetTileCount());
          
                ImGui::TreePop();
            }
            


            ImGui::End();



            ImGui::Begin("Settings");
            ImGui::Checkbox("Show colliders", &m_showColliders);
            ImGui::Checkbox("Show areas", &m_showAreas);
            ImGui::Checkbox("Show grid", &m_showGrid);
			ImGui::End();
            //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });


            ImGui::Begin("Viewport");


            ImVec2 imagePos = ImGui::GetItemRectMin(); // Where the actual scene is drawn
            ImVec2 mousePos = ImGui::GetMousePos();
            m_localMousePosInViewport = ImVec2(mousePos.x - imagePos.x, mousePos.y - imagePos.y);
			
            // Inside ImGui::Begin("Viewport") ...
            ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail(); // Area available for image
            ImVec2 viewportOrigin = ImGui::GetCursorScreenPos();       // Top-left of Image in screen coords

            m_viewportOrigin = { viewportOrigin.x, viewportOrigin.y };
            m_viewportBounds[0] = m_viewportOrigin;
            m_viewportBounds[1] = {
                m_viewportOrigin.x + viewportPanelSize.x,
                m_viewportOrigin.y + viewportPanelSize.y
            };

            // Optional debug log
           
            // Make sure viewport size is correct as well
            m_sceneHierarchyPanel.GetEditorScene()->OnViewportResize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y, m_viewportBounds);

            uint32_t newWidth = (uint32_t)viewportPanelSize.x;
            uint32_t newHeight = (uint32_t)viewportPanelSize.y;

		
            if ((uint32_t)m_viewportSize.x != newWidth || (uint32_t)m_viewportSize.y != newHeight)
            {
                m_viewportSize = { (float)newWidth, (float)newHeight };
                //m_editorScene->OnViewportResize(newWidth, newHeight, m_viewportBounds);
            }


            
            if (m_editor)
            {
                // Ensure that GetColorAttachmentRendererID() is valid
                VkDescriptorSet currentSet = Renderer::GetCurrentGameDescriptorSet();

                // 2. Cast it for ImGui
                ImTextureID textureID = (ImTextureID)currentSet;
                if (textureID != 0 && currentSet != VK_NULL_HANDLE)
                { 
                    ImGui::Image(textureID, ImVec2{ m_viewportSize.x, m_viewportSize.y }, ImVec2{ 0,1 }, ImVec2{ 1, 0 });
                }
                else
                {
                    EE_CORE_ERROR("Invalid texture ID: {}", textureID);
                }
                   
            }
            else 
            {
                
                uint32_t textureID = 0;// = m_framebuffer->GetColorAttachmentRendererID();
                if (textureID != 0)
                { 
                    EE_PROFILE_SCOPE("Imgui Render viewport");

                    ImGui::Image(textureID, ImVec2{ m_viewportSize.x, m_viewportSize.y }, ImVec2{ 0,1 }, ImVec2{ 1, 0 });
                }
            }
            
            if (ImGui::BeginDragDropTarget())
            {
               if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
               {
                    const wchar_t* path = (const wchar_t*)payload->Data;

                    OpenScene(std::filesystem::path(AssetManager::GetAssetFolderPath()) / path);
               }
               ImGui::EndDragDropTarget();
            }


            // Guizmo
    
            Entity selectedEntity = m_sceneHierarchyPanel.GetSelectedEntity();
           // auto cameraEntity = m_editor.get()->GetGameLayer()->GetActiveGameScene()->GetPrimaryCameraEntity();

            // check if placing tiles. if yes, dont show imGuizmo to avoid misclicks
            bool isPLacingTiles = m_ActiveStroke != nullptr;
            bool showImQuizmo = false;
            if (selectedEntity && selectedEntity.HasComponent<TransformComponent>() &&
                  m_sceneHierarchyPanel.GetGuizmoType() != -1 && !isPLacingTiles && showImQuizmo)
            {
                EE_PROFILE_SCOPE("Guizmo");

                ImGuizmo::SetOrthographic(false);
                ImGuizmo::SetDrawlist();
                float windowWidth = (float)ImGui::GetWindowWidth();
                float windowHeight = (float)ImGui::GetWindowHeight();
                ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);
               
                // Runtime Camera
                //CameraComponent& cameraComp = cameraEntity.GetComponent<CameraComponent>();
                //const glm::mat4& cameraProjection = cameraComp.Camera.GetViewProjection();
                //glm::mat4 cameraView = glm::inverse(cameraEntity.GetComponent<TransformComponent>().GetTransform());

                //editor camera
                const glm::mat4& cameraProjection = m_editorCamera.GetProjectionMatrix();
                glm::mat4 cameraView = m_editorCamera.GetView();


                // Entity transform
                TransformComponent& transformComp = selectedEntity.GetComponent<TransformComponent>();
                glm::mat4 transform = transformComp.GetTransform();

                bool snap = Input::IsKeyPressed(Key::LeftControl);
                float snapValue = 0.5f;
                if (m_sceneHierarchyPanel.GetGuizmoType() == ImGuizmo::OPERATION::ROTATE)
                {
                    snapValue = 5.0f;
                }

                float snapValues[3] = { snapValue, snapValue, snapValue };


                ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
                    (ImGuizmo::OPERATION)m_sceneHierarchyPanel.GetGuizmoType(), ImGuizmo::LOCAL, glm::value_ptr(transform),
                    nullptr, snap ? snapValues : nullptr);

                if (ImGuizmo::IsUsing())
                {
                    glm::vec3 translation, scale;
                    glm::quat rotationQuat;

                    // Decompose transformation matrix
                    Math::DecomposeTransform(transform, translation, rotationQuat, scale);

                    // Compute rotation difference as quaternion
                    glm::quat deltaRotation = rotationQuat * glm::inverse(glm::quat(transformComp.Rotation));

                    // Apply changes
                    transformComp.Translation = translation;
                    transformComp.Rotation = glm::eulerAngles(deltaRotation) + transformComp.Rotation; // Convert back to Euler angles
                    transformComp.Scale = scale;
                }
            }

            ImGui::End();

            ImGui::EndMenuBar();
        }

        UI_Toolbar();
        //DrawAIPromptPanel();

        ImGui::End();


    }



    void EditorLayer::UI_Toolbar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });

        ImGui::Begin("##toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        float size = ImGui::GetWindowHeight() - 10.0f;

        // Centering the toolbar buttons
        float toolbarWidth = 2 * size;  // Adjust based on number of buttons
        float offsetX = (ImGui::GetWindowContentRegionMax().x - toolbarWidth) * 0.5f;
        ImGui::SetCursorPosX(offsetX);

        // Play Button
        Ref<VulkanTexture> icon;
       
        {
            icon = m_sceneState == eSceneState::Play ? m_iconPause : m_iconPlay;
            if (ImGui::ImageButton("##playbutton", (ImTextureID)icon->GetTextureDescriptor(), ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1)))
            {
                
                if (m_sceneState == eSceneState::Pause || m_sceneState == eSceneState::Edit)
                {
                    OnScenePlay();
                }
                else if (m_sceneState == eSceneState::Play)
                {
                    OnScenePause();
                }
            }
        }

        

        ImGui::SameLine(); // Move the next item to the same row

        // Stop Button
        if (ImGui::ImageButton("##stopbutton", (ImTextureID)m_iconStop->GetTextureDescriptor(), ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1)))
        {
            if (m_sceneState == eSceneState::Play || m_sceneState == eSceneState::Pause)
            {
                OnSceneStop();
            }
        }

        ImGui::PopStyleColor(1);
        ImGui::PopStyleVar(2);
        ImGui::End();
    }

    bool EditorLayer::DrawRotatedImageButton(ImTextureID texture, const ImVec2& size, float angleRad, const char* id)
    {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 center = ImVec2(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);

        float s = sinf(angleRad);
        float c = cosf(angleRad);
        float hsx = size.x * 0.5f;
        float hsy = size.y * 0.5f;

        ImVec2 offset[] = {
            ImVec2(-hsx, -hsy),
            ImVec2(hsx, -hsy),
            ImVec2(hsx,  hsy),
            ImVec2(-hsx,  hsy),
        };

        ImVec2 rotated[4];
        for (int i = 0; i < 4; i++)
        {
            rotated[i] = ImVec2(
                center.x + offset[i].x * c - offset[i].y * s,
                center.y + offset[i].x * s + offset[i].y * c
            );
        }

        ImGui::GetWindowDrawList()->AddImageQuad(
            texture,
            rotated[0], rotated[1], rotated[2], rotated[3],
            ImVec2(0, 0), ImVec2(1, 0), ImVec2(1, 1), ImVec2(0, 1)
        );

        ImGui::SetCursorScreenPos(pos); // Restore cursor
        ImGui::InvisibleButton(id, size);
        return ImGui::IsItemClicked();
    }



    void EditorLayer::OnScenePlay()
    {

        // remove all lights( editor light)
        m_sceneHierarchyPanel.GetEditorScene()->ForEach<DirectionalLightComponent, TransformComponent>(
            [&](Entity e, DirectionalLightComponent& dl, TransformComponent& transformcomp)
            {
                e.RemoveComponent<TransformComponent>();
                m_sceneHierarchyPanel.GetEditorScene()->DestroyEntity(e);
            });


		m_editorScene = Scene::Copy(m_sceneHierarchyPanel.GetEditorScene());


        if (m_sceneState != eSceneState::Pause)
        {
            m_editor.get()->GetGameLayer()->OnGameStart();
            m_debugPanel.SetGameContext(m_editor.get()->GetGameLayer()->GetActiveGameScene());

            m_sceneHierarchyPanel.SetSceneHierarchyPanelScene(m_editor.get()->GetGameLayer()->GetActiveGameScene());
        }
 
        m_sceneState = eSceneState::Play;
        DeselectEntity();
        m_sceneHierarchyPanel.SetSelectionLocked(false);
    }

    void EditorLayer::OnSceneStop()
    {      

        m_sceneState = eSceneState::Edit;



        m_editor.get()->GetGameLayer()->SetIsPlaying(false);
        m_editor.get()->GetGameLayer()->GetActiveGameScene()->OnRunTimeStop();
       // m_debugPanel.SetGameContext(m_editor.get()->GetGameLayer()->GetActiveGameScene());

        m_sceneHierarchyPanel.SetSceneHierarchyPanelScene(m_editorScene);
		m_editor.get()->GetGameLayer()->SetActiveScene(m_sceneHierarchyPanel.GetEditorScene());
        m_editor.get()->GetGameLayer()->OnGameStop();
       // m_editor.get()->GetGameLayer()->CopyToActiveScene(Scene::Combine(m_sceneHierarchyPanel.GetEditorScene(), m_editor.get()->GetGameLayer()->GetActiveGameScene()));
   
        SortIsometricTilesByY();

    }

    void EditorLayer::OnScenePause()
    {
        m_sceneState = eSceneState::Pause;
        m_editor.get()->GetGameLayer()->SetIsPlaying(false);
    }

    void EditorLayer::OnDuplicateEntity()
    {
        if (m_sceneState != eSceneState::Edit)
        {
            return;
        }

        Entity selectedEntity = m_sceneHierarchyPanel.GetSelectedEntity();
        if (selectedEntity)
        {
            m_sceneHierarchyPanel.GetEditorScene()->DuplicateEntity(selectedEntity);
        }
    }

    void EditorLayer::SortIsometricTilesByY()
    {
        auto& registry = m_editor->GetGameLayer()->GetActiveGameScene()->GetRegistry();

        auto getLayer = [&](entt::entity e) -> int {
            if (!registry.all_of<TileComponent>(e)) return 0;
            // Use the highest layer found in this entity's tiles
            int maxLayer = 0;
                for (const auto& t : registry.get<TileComponent>(e).tiles)
                {
                    if (t.Category == eTileCategory::Roofs)   maxLayer = std::max(maxLayer, 2);
                    else if (t.Category == eTileCategory::Buildings) maxLayer = std::max(maxLayer, 1);
                }
            return maxLayer;
            };

        // A) Entities: sort by layer first, then Y descending within layer
        registry.sort<TransformComponent>(
            [&](entt::entity a, entt::entity b)
            {
                int layerA = getLayer(a);
                int layerB = getLayer(b);
                if (layerA != layerB) return layerA < layerB; // lower layer drawn first
                const auto& ta = registry.get<TransformComponent>(a).Translation;
                const auto& tb = registry.get<TransformComponent>(b).Translation;
                if (ta.y != tb.y) return ta.y > tb.y;
                return (uint32_t)a < (uint32_t)b;
            }
        );

        // B) Tiles within each entity: category first, then Y descending
        auto view = registry.view<TileComponent, TransformComponent>();
        for (auto e : view)
        {
            auto& tc = view.get<TileComponent>(e);
            const auto& tr = view.get<TransformComponent>(e);

            std::stable_sort(tc.tiles.begin(), tc.tiles.end(),
                [&](const TileInfo& A, const TileInfo& B)
                {
                    int layerA = (A.Category == eTileCategory::Roofs) ? 2 :
                        (A.Category == eTileCategory::Buildings) ? 1 : 0;
                    int layerB = (B.Category == eTileCategory::Roofs) ? 2 :
                        (B.Category == eTileCategory::Buildings) ? 1 : 0;

                    if (layerA != layerB) return layerA < layerB; // lower layer drawn first

                    const float yA = tr.Translation.y + A.position.y;
                    const float yB = tr.Translation.y + B.position.y;
                    return yA > yB; // DESC by Y within same layer
                }
            );
        }
    }

   

    glm::vec2 EditorLayer::GetSnappedIsoPosition()
    {
        glm::vec2 ndc;
        ndc.x = (m_localMousePosInViewport.x / m_viewportSize.x) * 2.0f - 1.0f;
        ndc.y = 1.0f - (m_localMousePosInViewport.y / m_viewportSize.y) * 2.0f;

        glm::vec4 clipNear(ndc.x, ndc.y, -1.0f, 1.0f);
        glm::vec4 clipFar(ndc.x, ndc.y, 1.0f, 1.0f);

        glm::mat4 invViewProj = glm::inverse(m_editorCamera.GetViewProjection());
        glm::vec4 worldNear = invViewProj * clipNear; worldNear /= worldNear.w;
        glm::vec4 worldFar = invViewProj * clipFar;  worldFar /= worldFar.w;

        glm::vec3 ro = glm::vec3(worldNear);
        glm::vec3 rd = glm::normalize(glm::vec3(worldFar - worldNear));
        float t = -ro.z / rd.z;
        glm::vec3 hit = ro + t * rd;
        glm::vec2 p(hit.x, hit.y);

        // Snap to cell & compute its ground point
        glm::ivec2 isoCell = IsoTileUtils::WorldToIsoCellInt(p);
        return isoCell;
    }


    bool EditorLayer::CanPlaceTile(std::string selectedTileName, glm::ivec2 isoCell)
    {
        auto& registry = m_editor->GetGameLayer()->GetActiveGameScene()->GetRegistry();
        {
            auto view = registry.view<TileComponent, TransformComponent, IDComponent>();
            for (auto entity : view)
            {
                const auto& tc = view.get<TileComponent>(entity);
                const auto& tr = view.get<TransformComponent>(entity);
                for (const auto& tinfo : tc.tiles)
                {
                    glm::vec2 tileGround = glm::vec2(tr.Translation) + tinfo.position;
                    if (IsoTileUtils::WorldToIsoCellInt(tileGround) == isoCell &&
                        tinfo.name == selectedTileName)
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    TileInfo EditorLayer::OnCreateTileEntity(std::string selectedTileName, glm::vec4 UV, eTileCategory tileCategory)
    {
        // Ray  world Z=0 (unchanged)
        glm::ivec2 isoCell = GetSnappedIsoPosition();
        glm::vec2  groundPos = IsoTileUtils::IsoToWorldGround(isoCell);
        TileInfo newTile;

        // Duplicate check (compare iso cells)
        

        // Flags
        bool destructible = (tileCategory == eTileCategory::Buildings);
        bool isRoof = (tileCategory == eTileCategory::Roofs);
        TileProperties& tileProps = m_tileEditorPanel.GetSelectedTileProperties();

       
        if (!m_selectedEntity)
        {
            // try to get it from scenehierachy 
            m_selectedEntity = m_sceneHierarchyPanel.GetSelectedEntity();
        }

        // Place
        if (m_selectedEntity)
        {
            if (!m_selectedEntity.HasComponent<TransformComponent>())
            {
                //the enttiy was removed, if selected entity does not have transformcomponent, 
                // this made crash. there is probably better fix somewhere out there
                return newTile;
            }

            TransformComponent& tr = m_selectedEntity.GetComponent<TransformComponent>();
            IDComponent& idComp = m_selectedEntity.GetComponent<IDComponent>();
            glm::ivec2 baseIso = IsoTileUtils::WorldToIsoCellInt(glm::vec2(tr.Translation));
            glm::ivec2 localIso = isoCell - baseIso;

            // Store WORLD delta to the target cell's **ground**
            const glm::vec2 deltaGround = IsoTileUtils::IsoDeltaToWorldDeltaGround(localIso);
            eTileDirection tileDir = EditorUtils::GetDirectionFromTileName(selectedTileName);

            uint64_t tileID = HashUtils::MakeTileUID(
                (uint64_t)idComp.ID,
                groundPos,
                float(TILE_SIZE),
                static_cast<uint32_t>(tileCategory),
                tileDir
            );

            EE_CORE_WARN(
                "TileUID inputs -> entID: {}, pos: ({:.3f}, {:.3f}), tileSize: {:.3f}, category: {}, direction: {}",
                (uint64_t)idComp.ID,
                groundPos.x, groundPos.y,
                float(TILE_SIZE),
                static_cast<uint32_t>(tileCategory),
                static_cast<uint32_t>(tileDir)
            );

            auto& tc = m_selectedEntity.GetComponent<TileComponent>();
            newTile;
            newTile.position = deltaGround;
            newTile.UV = UV;
            newTile.name = selectedTileName;
            newTile.IsDestructible = destructible;
            newTile.IsRoof = isRoof;
            newTile.Category = tileCategory;
            newTile.Material = tileProps.material;
            newTile.TileHealth = tileProps.health;
            newTile.UID = tileID;

            newTile.TileDirection = EditorUtils::GetDirectionFromTileName(selectedTileName);

            if (tileCategory == eTileCategory::Buildings || tileCategory == eTileCategory::Pillars)
            {
                newTile.IsSupportingRoof = true;
            }
            else
            {
                newTile.IsSupportingRoof = false;
            }


            tc.tiles.push_back(newTile);
            EE_CORE_INFO("adding tile: {}", tc.tiles.size());
        }
        else
        {
            // New entity anchored at **ground**
            Entity newEntity = m_editor->GetGameLayer()->GetActiveGameScene()->CreateEntity();
            TransformComponent& transformCmp = newEntity.AddComponent<TransformComponent>();
            IDComponent& idComp = newEntity.GetComponent<IDComponent>();

            transformCmp.Translation.x = groundPos.x;
            transformCmp.Translation.y = groundPos.y;
            eTileDirection tileDir = EditorUtils::GetDirectionFromTileName(selectedTileName);

            uint64_t tileID  = HashUtils::MakeTileUID(
                (uint64_t)idComp.ID,
                groundPos,
                float(TILE_SIZE),
                static_cast<uint32_t>(tileCategory),
                tileDir
            );
            
            TileComponent& tileComp = newEntity.AddComponent<TileComponent>();
            
            newTile.position = glm::vec3(0);
            newTile.UV = UV;
            newTile.name = selectedTileName;
            newTile.IsDestructible = destructible;
            newTile.IsRoof = isRoof;
            newTile.Category = tileCategory;
            newTile.Material = tileProps.material;
            newTile.TileHealth = tileProps.health;
            newTile.UID = tileID;
            
            if (tileCategory == eTileCategory::Buildings || tileCategory == eTileCategory::Pillars)
            {
                newTile.IsSupportingRoof = true;
            }
            else
            {
                newTile.IsSupportingRoof = false;
            }

            // 2. Set the direction using the helper function we made
            newTile.TileDirection = EditorUtils::GetDirectionFromTileName(selectedTileName);



            // 4. Push it to the component
            tileComp.tiles.push_back(newTile);


            m_selectedEntity = newEntity;
            m_sceneHierarchyPanel.SetSelectedEntity(newEntity);

        }

        SortIsometricTilesByY();

        return newTile;
    }
    CompactTile EditorLayer::BuildCompactTileForSelection(Entity selectedEntity, glm::ivec2 isoCell)
    {
        Ref<Scene> scene = m_editor->GetGameLayer()->GetActiveGameScene();
        if (!scene)
        {
            EE_CORE_WARN("BuildCompactTileForSelection scene = null");
            return {};
        }

        const uint16_t typeId = GetOrCreateDefinitionForSelectedTile();
        if (typeId == 0)
        {
            EE_CORE_WARN("BuildCompactTileForSelection typeId == 0");
            return {};
        }

        if (m_sceneHierarchyPanel.IsSelectionLocked())
            selectedEntity = m_sceneHierarchyPanel.GetSelectedEntity();

        uint64_t groupID = 0;

        if (!selectedEntity || !scene->IsEntityValid(selectedEntity))
        {
            Entity newEntity = scene->CreateEntity("Entity");
            selectedEntity = newEntity;
            m_selectedEntity = newEntity;
            m_sceneHierarchyPanel.SetSelectedEntity(newEntity);

            groupID = static_cast<uint64_t>(newEntity.GetUUID());
            newEntity.GetComponent<TagComponent>().Tag = "Entity" + std::to_string(groupID);
        }
        else
        {
            groupID = static_cast<uint64_t>(selectedEntity.GetUUID());
        }

        CompactTile tile{};
        tile.TypeId = typeId;
        tile.Flags = Engine::CompactTileFlags::None;
        tile.Aux = 0;
        tile.GroupId = groupID;

        CompactTileMap& compactMap = scene->GetCompactTileMap();

        if (compactMap.HasTileType(isoCell, tile.TypeId))
        {
            EE_CORE_INFO("Compact tile type {} already exists at cell ({}, {})",
                tile.TypeId, isoCell.x, isoCell.y);
            return {};
        }

        return tile;
    }

    uint16_t EditorLayer::GetOrCreateDefinitionForSelectedTile()
    {
        Ref<Scene> scene = m_editor->GetGameLayer()->GetActiveGameScene();
        if (!scene)
            return 0;

        Engine::TileDefinitionRegistry& defs = scene->GetTileDefinitions();

        const std::string selectedTileName = m_tileEditorPanel.GetSelectedTileName();




        const glm::vec4 selectedUV = m_tileEditorPanel.GetTileUV(selectedTileName);
        const eTileCategory selectedCategory = m_tileEditorPanel.GetSelectedTileCategory();
        const TileProperties& tileProps = m_tileEditorPanel.GetSelectedTileProperties();

        if (selectedTileName.empty())
        {
            EE_CORE_WARN("GetOrCreateDefinitionForSelectedTile: no tile selected");
            return 0;
        }

        TileInfo temp{};
        temp.name = selectedTileName;
        temp.UV = selectedUV;
        temp.Category = selectedCategory;
        temp.TileDirection = EditorUtils::GetDirectionFromTileName(selectedTileName);;

        Engine::TileTypeKey key = TileManager::MakeTileTypeKey(temp);
        // If MakeTileTypeKey is non-static member, move that helper out somewhere shared.
        // Or build key manually here.

        uint16_t existingTypeId = 0;
        if (defs.FindTypeId(key, existingTypeId))
            return existingTypeId;

        Engine::TileDefinition def{};
        def.TypeId = defs.GetNextTypeId();
        def.Name = selectedTileName;
        def.UV = selectedUV;
        def.Category = selectedCategory;
        def.Direction = temp.TileDirection;
        def.Material = tileProps.material;
        def.BaseHealth = static_cast<uint16_t>(tileProps.health);
        def.IsDestructible = (selectedCategory == eTileCategory::Buildings);
        def.IsSupportingRoof = (selectedCategory == eTileCategory::Buildings || selectedCategory == eTileCategory::Pillars);
        def.IsRoof = (selectedCategory == eTileCategory::Roofs);

        if (!defs.Register(def, key))
        {
            EE_CORE_WARN("GetOrCreateDefinitionForSelectedTile: failed to register '{}'", selectedTileName);
            return 0;
        }

        return def.TypeId;
    }





    void EditorLayer::OnOverlayRender()
    {
        if (m_sceneState == eSceneState::Play)
        {
            Entity camera = m_editor.get()->GetGameLayer()->GetActiveGameScene()->GetPrimaryCameraEntity();
            if (!camera)
            {
                return;
            }
          // Engine::VulkanRenderer2D::BeginScene(camera.GetComponent<CameraComponent>().Camera, camera.GetComponent<TransformComponent>().GetTransform(), true);

        }
        else
        {
            Engine::VulkanRenderer2D::BeginScene(m_editorCamera);

        }

        std::string selectedTile = m_tileEditorPanel.GetSelectedTileName();

        if (m_mouseIsInViewPort && !selectedTile.empty() && m_sceneState == eSceneState::Edit)
        {
            // preview tile placement renderiong
            glm::vec2 ndc;
            ndc.x = (m_localMousePosInViewport.x / m_viewportSize.x) * 2.0f - 1.0f;
            ndc.y = 1.0f - (m_localMousePosInViewport.y / m_viewportSize.y) * 2.0f;

            glm::vec4 clipNear(ndc.x, ndc.y, -1.0f, 1.0f);
            glm::vec4 clipFar(ndc.x, ndc.y, 1.0f, 1.0f);

            glm::mat4 invViewProj = glm::inverse(m_editorCamera.GetViewProjection());
            glm::vec4 worldNear = invViewProj * clipNear; worldNear /= worldNear.w;
            glm::vec4 worldFar = invViewProj * clipFar;  worldFar /= worldFar.w;

            glm::vec3 ro = glm::vec3(worldNear);
            glm::vec3 rd = glm::normalize(glm::vec3(worldFar - worldNear));
            float t = -ro.z / rd.z;
            glm::vec3 hit = ro + t * rd;
            glm::vec2 p(hit.x, hit.y);

            // Grid sizes (diamond), NOT sprite height
            const float tileW = float(TILE_SIZE);        // 128
            const float tileH = tileW * 0.5f;            // 64

            auto WorldToIsoCell = [&](const glm::vec2& wp) -> glm::ivec2 {
                float tX = wp.x / (tileW * 0.5f);
                float tY = wp.y / (tileH * 0.5f);
                float u = 0.5f * (tY + tX);
                float v = 0.5f * (tY - tX);
                return glm::ivec2(glm::round(glm::vec2(u, v)));
                };

            auto IsoToWorldGround = [&](glm::ivec2 c) -> glm::vec2 {
                return {
                    (c.x - c.y) * (tileW * 0.5f),
                    (c.x + c.y) * (tileH * 0.5f)
                };
                };

            glm::ivec2 isoCell = WorldToIsoCell(p);
            glm::vec2  ground = IsoToWorldGround(isoCell);

            glm::vec4 uv = m_tileEditorPanel.GetTileUV(selectedTile);
           // glm::vec4 previewUV(uv.x, uv.w, uv.z, uv.y); // flip V
            glm::vec4 previewColor(0.3f, 1.0f, 0.3f, 0.55f);

            // IMPORTANT: pass the ground point; DrawTile must be bottom-center pivot
            Engine::VulkanRenderer2D::DrawTile(ground, uv, previewColor);
        
        
        }

        if (m_sceneState == eSceneState::Edit)
        {
            DrawSelectedTileOutline();
        }

        if (m_showAreas)
        {
            EditorDebugUtils::DrawAreaDebugBounds(m_editor.get()->GetGameLayer()->GetActiveGameScene());
             
             
            
       


        }
        if(m_showGrid)
        {
           
            glm::vec2 minW, maxW;
            m_editorCamera.GetViewportWorldBounds2D(minW, maxW, 0.0f);

            glm::vec2 cameraPos2D = glm::vec2{ m_editorCamera.GetPosition().x, m_editorCamera.GetPosition().y };
            float extent = 0.5f * std::max(maxW.x - minW.x, maxW.y - minW.y);
            
            float isometricScale = 1.5f;
            EditorDebugUtils::DrawIsometricGrid(cameraPos2D, extent * isometricScale);
            EditorDebugUtils::DrawWorldAxes(cameraPos2D, extent);

        }




        
        if (m_showColliders)
        {
            {
                auto view = m_editor.get()->GetGameLayer()->GetActiveGameScene()->GetAllEntitiesWith<TransformComponent, CircleCollider2DComponent>();

                for (auto entity : view)
                {
                    auto [transformComp, cirlceColliderComp] = view.get<TransformComponent, CircleCollider2DComponent>(entity);

                    glm::vec3 translation = transformComp.Translation + glm::vec3(cirlceColliderComp.Offset, 0.1f);
                    glm::vec3 scale = transformComp.Scale * glm::vec3(cirlceColliderComp.Radius * 2.0f);


                    glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation) *
                        glm::scale(glm::mat4(1.0f), scale);



                   // Renderer2D::DrawCircle(transform, glm::vec4(0.0f, 0.9f, 0.0f, 1.0f), 0.1f);
                }
            }

            {
                auto view = m_editor.get()->GetGameLayer()->GetActiveGameScene()->GetAllEntitiesWith<TransformComponent, BoxCollider2DComponent>();

                for (auto entity : view)
                {
                    auto [transformComp, boxColliderComp] = view.get<TransformComponent, BoxCollider2DComponent>(entity);

                    glm::vec3 translation = transformComp.Translation + glm::vec3(boxColliderComp.Offset, 0.1f);
                    glm::vec3 scale = transformComp.Scale * glm::vec3(boxColliderComp.Size * 2.0f, 1.0f);


                    glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation)
                        * glm::rotate(glm::mat4(1.0f),transformComp.Rotation.z , glm::vec3(0.0f, 0.0f, 1.0f))
                        * glm::scale(glm::mat4(1.0f), scale);


					Engine::VulkanRenderer2D::DrawLineRect(transform, glm::vec4(0.1f, 0.9f, 0.1f, 1.0f), -1);
                }

            }

        }
       
        Engine::VulkanRenderer2D::EndScene();
    }


    void EditorLayer::OnUpdate(Engine::Timestep timestep)
    {
        EE_PROFILE_FUNCTION();

    

        m_controlPressed = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
        m_editorCamera.OnUpdate(timestep);

        m_fpsCounter.Update(timestep);

        // ******** Render ***********

        {
            EE_PROFILE_SCOPE("render draw");
            //********* update scene *********

            switch (m_sceneState)
            {
            case Engine::EditorLayer::eSceneState::Edit:
            {
                glm::vec2 viewMinWorld, viewMaxWorld;
                m_editorCamera.GetViewportWorldBounds2D(viewMinWorld, viewMaxWorld, 0.0f);

                float compactMarginWorld = 10.0f;


                m_editor.get()->GetGameLayer()->GetActiveGameScene()->GetCompactTilePromotion().EnsurePromotedInEditorViewport(
                    m_editor.get()->GetGameLayer()->GetActiveGameScene().get(),
                    viewMinWorld,
                    viewMaxWorld,
                    compactMarginWorld,
                    m_editor.get()->GetGameLayer()->GetActiveGameScene()->GetTileManager());


                // m_editor.get()->GetGameLayer()->GetActiveGameScene()->GetCompactTileMap().Render(m_editor.get()->GetGameLayer()->GetActiveGameScene()->GetTileDefinitions());
                m_editor.get()->GetGameLayer()->GetActiveGameScene()->OnUpdateEditor(timestep, m_editorCamera);
                break;

            }
            case Engine::EditorLayer::eSceneState::Play:
            {
                //m_editor.get()->GetGameLayer()->GetActiveGameScene()->OnUpdateRuntime(timestep);
                break;
            }
            case Engine::EditorLayer::eSceneState::Pause:
            {
                m_editor.get()->GetGameLayer()->GetActiveGameScene()->OnUpdateRuntime(timestep, false);
                break;
            }
            }


            ImVec2 mousePos = ImGui::GetMousePos();
            mousePos.x -= m_viewportBounds[0].x;
            mousePos.y -= m_viewportBounds[0].y;
            glm::vec2 viewportSize = m_viewportBounds[1] - m_viewportBounds[0];

            mousePos.y = viewportSize.y - mousePos.y;
            int mouseX = (int)mousePos.x;
            int mouseY = (int)mousePos.y;

            m_mouseIsInViewPort = mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y;

            OnOverlayRender();


            if (m_mouseIsInViewPort && m_sceneState == eSceneState::Edit)
            {

                if (Input::IsMouseButtonPressed(Mouse::Button0))
                {
                    PlaceSelectedTile();
                }
                
            }
        }
    }

    void EditorLayer::PlaceSelectedTile()
    {

        std::string selectedTile = m_tileEditorPanel.GetSelectedTileName();
        if (selectedTile.empty())
        {
            return;
        }

        Entity selectedEntity = m_selectedEntity;
        if (!m_ActiveStroke)
        {
            m_ActiveStroke = std::make_unique<CommandGroup>();

            if (!selectedEntity || m_sceneHierarchyPanel.IsSelectionLocked())
            {
                // and this from hierarchy?!
                selectedEntity = m_sceneHierarchyPanel.GetSelectedEntity();
            }

            m_StrokeCreatedNewEntity = !selectedEntity;
        }
        else
        {
            if (!m_ActiveStroke->IsEmpty())
            {
                m_CommandHistory.Push(std::move(m_ActiveStroke));
            }
            m_ActiveStroke = nullptr;
            return;
        }

        glm::ivec2 isoCell = GetSnappedIsoPosition();
        glm::vec2  snapped = IsoTileUtils::IsoToWorldGround(isoCell);
        
        // check if there us same tile in same position
        if (CanPlaceTile(selectedTile, isoCell))
        {

            
            
            CompactTile compactTile = BuildCompactTileForSelection(selectedEntity, isoCell);
            if (compactTile.IsEmpty())
                return;

            eTileDirection tileDir = EditorUtils::GetDirectionFromTileName(selectedTile);

            Scope<PlaceCompactTileCommand> cmd = std::make_unique<PlaceCompactTileCommand>(m_editor.get()->GetGameLayer()->GetActiveGameScene().get(),
                isoCell, compactTile, tileDir);
            cmd->Execute();
            m_ActiveStroke->AddCommand(std::move(cmd));
            
            EE_CORE_INFO(" placeed {}", isoCell);

            m_LastPlacedTilePos = snapped;
            m_StrokeCreatedNewEntity = false; 
        }
            

    }

    void EditorLayer::DrawSelectedTileOutline()
    {
        Entity entity = m_sceneHierarchyPanel.GetSelectedEntity();
        if (!entity || !m_sceneHierarchyPanel.GetEditorScene()->IsEntityValid(entity))
            return;

        if (!entity.HasComponent<TransformComponent>() || !entity.HasComponent<TileComponent>())
            return;

        const std::optional<size_t>& selectedIdx = m_sceneHierarchyPanel.GetSelectedTileIndex();
        if (!selectedIdx.has_value())
            return;

        auto& tc = entity.GetComponent<TileComponent>();
        if (*selectedIdx >= tc.tiles.size())
            return;

        auto& transform = entity.GetComponent<TransformComponent>();
        const TileInfo& tile = tc.tiles[*selectedIdx];

        if (tile.opaqueMin.x == 999 || tile.opaqueMin.y == 999 ||
            tile.opaqueMax.x == 999 || tile.opaqueMax.y == 999)
        {
            return;
        }

        const glm::vec2 center = glm::vec2(transform.Translation) + tile.position;

        constexpr float tileWorldW = TILE_SIZE;
        constexpr float tileWorldH = TILE_SIZE * 2.0f;

        constexpr float pixelToWorldX = tileWorldW / float(TILE_PIXEL_WIDTH);
        constexpr float pixelToWorldY = tileWorldH / float(TILE_PIXEL_HEIGHT);

        const float opaqueW = float(tile.opaqueMax.x - tile.opaqueMin.x) * pixelToWorldX;
        const float opaqueH = float(tile.opaqueMax.y - tile.opaqueMin.y) * pixelToWorldY;

        const float halfFaceW = opaqueW * 0.5f;
        const float faceH = opaqueH;


        const glm::vec2 bottomCenter = center;

        glm::vec2 faceDir(0.0f);
        switch (tile.TileDirection)
        {
        case eTileDirection::West:
            faceDir = glm::normalize(glm::vec2(-1.0f, 0.5f));
            break;
        case eTileDirection::South:
            faceDir = glm::normalize(glm::vec2(1.0f, 0.5f));
            break;
        case eTileDirection::East:
            faceDir = glm::normalize(glm::vec2(1.0f, -0.5f));
            break;
        case eTileDirection::North:
            faceDir = glm::normalize(glm::vec2(-1.0f, -0.5f));
            break;
        default:
            faceDir = glm::normalize(glm::vec2(1.0f, 0.5f));
            break;
        }


        const glm::vec2 up(0.0f, faceH);
        const glm::vec2 side = faceDir * halfFaceW;

        // Diamond/parallelogram face
        const glm::vec2 p0 = bottomCenter - side;       // bottom left on face
        const glm::vec2 p1 = bottomCenter + side;       // bottom right on face
        const glm::vec2 p2 = bottomCenter + side + up;  // top right
        const glm::vec2 p3 = bottomCenter - side + up;  // top left

        const glm::vec4 color(1.0f, 1.0f, 0.0f, 1.0f);

        VulkanRenderer2D::DrawLine(glm::vec3(p0, 0.0f), glm::vec3(p1, 0.0f), color);
        VulkanRenderer2D::DrawLine(glm::vec3(p1, 0.0f), glm::vec3(p2, 0.0f), color);
        VulkanRenderer2D::DrawLine(glm::vec3(p2, 0.0f), glm::vec3(p3, 0.0f), color);
        VulkanRenderer2D::DrawLine(glm::vec3(p3, 0.0f), glm::vec3(p0, 0.0f), color);
    }



    void EditorLayer::OnUpdateECS(Timestep timestep)
    {
		EE_PROFILE_FUNCTION();
    }

    void EditorLayer::OnEvent(Engine::Event& event)
    {
        if (m_mouseIsInViewPort)
        {
            // block events if mouse is not in the game viewport
            // I did this because I wanted to block scrolling if mouse is not in viewport
            // might block some other events that should not be blocked
            m_editorCamera.OnEvent(event);
        }

        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<KeyPressedEvent>(EE_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
        dispatcher.Dispatch<MouseButtonPressedEvent>(EE_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));

    }

    bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
    {
        if (m_sceneState == eSceneState::Play)
        {
            return false;
        }

        if (e.GetMouseButton() == Mouse::Button0)
        {
            std::string selectedTile = m_tileEditorPanel.GetSelectedTileName();
            

            if (!ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt) && selectedTile == "" && m_mouseIsInViewPort)
            {

                // quick way to get clicked entity
                // better would be to add entity ID to vertex data
                // and read it from shaders.
                glm::vec2 ndc;
                ndc.x = (m_localMousePosInViewport.x / m_viewportSize.x) * 2.0f - 1.0f;
                ndc.y = 1.0f - (m_localMousePosInViewport.y / m_viewportSize.y) * 2.0f;

                glm::vec4 clipNear(ndc.x, ndc.y, -1.0f, 1.0f);
                glm::vec4 clipFar(ndc.x, ndc.y, 1.0f, 1.0f);
                glm::mat4 invViewProj = glm::inverse(m_editorCamera.GetViewProjection());
                glm::vec4 worldNear = invViewProj * clipNear;
                glm::vec4 worldFar = invViewProj * clipFar;
                worldNear /= worldNear.w;
                worldFar /= worldFar.w;

                glm::vec3 rayOrigin = glm::vec3(worldNear);
                glm::vec3 rayDir = glm::normalize(glm::vec3(worldFar - worldNear));
                float t = -rayOrigin.z / rayDir.z; // When Z = 0
                glm::vec3 hitPoint = rayOrigin + t * rayDir;
                glm::vec2 finalWorldPos = glm::vec2(hitPoint.x, hitPoint.y);
                glm::vec2 snapped;
                snapped.x = std::round(finalWorldPos.x);
                snapped.y = std::round(finalWorldPos.y);

                Entity foundEntity = EditorUtils::FindEntityAtPosition(m_sceneHierarchyPanel.GetEditorScene(), snapped);
                
                if (!m_sceneHierarchyPanel.IsSelectionLocked())
                {
                    m_selectedEntity = foundEntity;
                    if (m_selectedEntity)
                    {
                        m_sceneHierarchyPanel.SetSelectedEntity(m_selectedEntity);
                        EE_CORE_INFO("viewport Clicked ID:  {}", (uint64_t)m_selectedEntity.GetComponent<IDComponent>().ID);
                    }
                }
               
               
                if (!m_selectedEntity)
                {
                   // EditorDebugUtils::PrintAllEntities(m_sceneHierarchyPanel.GetNewComponentsContext()->GetRegistry());

                    m_selectedEntity = m_sceneHierarchyPanel.GetSelectedEntity();
                    
                   // m_selectedEntity = EditorUtils::FindTileAtPosition(m_editor.get()->GetGameLayer()->GetActiveGameScene(), snapped);
                    m_sceneHierarchyPanel.SetSelectedEntity(m_selectedEntity);
                }

                if (m_selectedEntity && m_selectedEntity.HasComponent<TileComponent>())
                {
                    TileComponent& tileComp = m_selectedEntity.GetComponent<TileComponent>();
                    TransformComponent& transformComp = m_selectedEntity.GetComponent<TransformComponent>();
                    auto indexOpt = EditorUtils::FindTileIndexAtPosition(tileComp, transformComp, snapped);
                    if (indexOpt)
                    {
                        m_sceneHierarchyPanel.SetSelectedTileIndex(*indexOpt);
                    }
                }

                if (!m_selectedEntity)
                {
                    return false; // No entity found at the clicked position
                }
				m_selectedTilePosition = snapped;
                return true;
            }
            else if (m_mouseIsInViewPort && !ImGuizmo::IsOver() && !m_hoveredEntity)
            {
                // reset selected entity
                //m_sceneHierarchyPanel.SetSelectedEntity({}); 
                //return true;
            }
        }
        else if (e.GetMouseButton() == Mouse::Button1)
        {
      
            DeselectEntity();
            
		}
		else if (e.GetMouseButton() == Mouse::Button2)
		{
			// Right click - deselect tile
			m_tileEditorPanel.SetSelectedTile(UINT_MAX, "");
        }
        return false;
    }

    void EditorLayer::DeselectEntity()
    {
        m_tileEditorPanel.SetSelectedTile(UINT_MAX, "");
        m_hoveredEntity = Entity(); // Reset hovered entity
       
        if (m_sceneHierarchyPanel.IsSelectionLocked())
            return; // Don't allow deselection if locked
       
        // Deselect
        m_sceneHierarchyPanel.SetSelectedEntity({}); // Deselect entity
        m_selectedEntity = Entity(); // Reset selected entity
    }


    bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
    {
        // shortcuts
        if (e.IsRepeat())
        {
            return false;
        }

        bool shiftPressed = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);

        switch (e.GetKeyCode())
        {
        
        case Key::Z:
        {
            if (m_controlPressed && !shiftPressed)
            {
                m_CommandHistory.Undo();
                return true;
            }
            
            break;
        }
        case Key::Y:
        {
            if (m_controlPressed)
            {
                m_CommandHistory.Redo();
                return true;
            }
            break;
        }
        


        case Key::N:
        {
            if (m_controlPressed)
            {
                NewScene();
            }
            break;
        }
        case Key::S:
        {
            if (m_controlPressed && shiftPressed)
            {
                SaveSceneAs();
            }
            else if (m_controlPressed)
            {
                SaveScene();
            }
            break;
        }
        case Key::O:
        {
            if (m_controlPressed)
            {
                OpenScene();
            }
            break;
        }
        case Key::D:
        {
            if (m_controlPressed)
            {
                OnDuplicateEntity();
            }
            break;
        }


        //Gizmos
        case Key::Q:
        {
            m_sceneHierarchyPanel.SetGizmoType(-1);
            break;
        }
        case Key::W:
        {
            m_sceneHierarchyPanel.SetGizmoType(ImGuizmo::OPERATION::TRANSLATE);
            break;
        }
        case Key::E:
        {
            m_sceneHierarchyPanel.SetGizmoType(ImGuizmo::OPERATION::ROTATE);
            break;
        }
        case Key::R:
        {
            m_sceneHierarchyPanel.SetGizmoType(ImGuizmo::OPERATION::SCALE);
            break;
        }
        case Key::Delete:
        {
            if (!m_selectedEntity)
            {
                break;
            }
            if (m_selectedEntity.HasComponent<TileComponent>())
            {
                TileComponent& tileComp = m_selectedEntity.GetComponent<TileComponent>();
                if (tileComp.tiles.size() == 0)
                {
                    m_sceneHierarchyPanel.DestrtoySelectedEntity(m_selectedEntity);
                }
                
                else
                {
					EditorUtils::DeleteTileAtPosition(m_selectedEntity, m_selectedTilePosition);
                }
            }
            else
            {
                m_sceneHierarchyPanel.DestrtoySelectedEntity(m_selectedEntity);

            }

            break;
        }
        }
        return false;
    }

    

    void EditorLayer::NewScene()
    {
        m_currentScenePath = std::filesystem::path();
    }

    void EditorLayer::OpenScene()
    {
        std::string filepath = FileDialogs::OpenFile("Engine scene (*.ee)\0*.ee\0");
        if (!filepath.empty())
        {
            OpenScene(filepath);
        }
    }

    void EditorLayer::LoadPrefab()
    {
        std::string filepath = FileDialogs::OpenFile("Prefab (*.prefab)\0*.prefab\0");
        if (filepath.empty())
            return;

        Ref<Scene> scene = m_sceneHierarchyPanel.GetEditorScene();
        if (!scene)
        {
            EE_CORE_WARN("LoadPrefab failed, editor scene is null");
            return;
        }

        glm::ivec2 spawnCell = GetSnappedIsoPosition();

        PrefabSerializer serializer(scene);
        Entity newEntity = serializer.Deserialize(filepath, spawnCell);

        if (newEntity)
        {
            m_selectedEntity = newEntity;
            m_sceneHierarchyPanel.SetSelectedEntity(newEntity);
        }
    }

    void EditorLayer::OpenScene(const std::filesystem::path& path)
    {
        m_sceneState = eSceneState::Edit;



        

        if (m_sceneState != eSceneState::Edit)
        {
        }

        if (path.extension().string() != ".ee")
        {
            EE_CORE_WARN("could not load {} - not s scene file .ee", path.filename().string());
            return;
        }

        m_sceneHierarchyPanel.GetEditorScene() = nullptr;
        m_sceneHierarchyPanel.GetEditorScene() = std::make_shared<Scene>();

        SceneSerializer serializer(m_sceneHierarchyPanel.GetEditorScene());
        serializer.Deserialize(path.string());


        //m_sceneHierarchyPanel.GetEditorScene() = Scene::Copy(m_editorScene);


        m_currentScenePath = path;
        
        m_editor.get()->GetGameLayer()->SetIsPlaying(false);
        m_editor.get()->GetGameLayer()->GetActiveGameScene()->OnRunTimeStop();
         m_debugPanel.SetGameContext(m_editor.get()->GetGameLayer()->GetActiveGameScene());

        m_sceneHierarchyPanel.SetSceneHierarchyPanelScene(m_sceneHierarchyPanel.GetEditorScene());

        m_editor.get()->GetGameLayer()->SetActiveScene(m_sceneHierarchyPanel.GetEditorScene());
        m_editor.get()->GetGameLayer()->OnGameStop();
    }

    void EditorLayer::SaveSceneAs()
    {
        std::string filepath = FileDialogs::SaveFile("Engine scene (*.ee)\0*.ee\0");
        if (!filepath.empty())
        {
            SceneSerializer serializer(m_sceneHierarchyPanel.GetEditorScene());
            serializer.Serialize(filepath);
            m_currentScenePath = filepath;
        }
    }



    void EditorLayer::SaveScene()
    {
        if (!m_currentScenePath.empty())
        {
            //EditorDebugUtils::PrintAllEntities(m_sceneHierarchyPanel.GetEditorScene().get());

            SceneSerializer serializer(m_sceneHierarchyPanel.GetEditorScene());

            // Log before saving to check what exists in the scene
            //DebugUtils::LogAllEntitiesWithComponents(m_editorScene);
            //DebugUtils::LogAllEntitiesWithComponents(m_sceneHierarchyPanel.GetNewComponentsContext());

            // Serialize the current editor scene without reloading it
            serializer.Serialize(m_currentScenePath.string());
        }
		else
		{
            
        }
		
    }

    void EditorLayer::SaveAsPrefab()
    {
        Entity selection = m_sceneHierarchyPanel.GetSelectedEntity();

        if (!selection )
        {
            EE_CORE_WARN("No entity selected for prefab!");
            return;
        }

        std::string filepath = FileDialogs::SaveFile("Prefab (*.prefab)\0*.prefab\0");

        if (filepath.empty())
            return;

        // Ensure extension
        if (filepath.find(".prefab") == std::string::npos)
            filepath += ".prefab";

        PrefabSerializer serializer(m_sceneHierarchyPanel.GetEditorScene());
        serializer.Serialize(filepath, selection);

        EE_CORE_INFO("Prefab saved: {}", filepath);
    }
    


}