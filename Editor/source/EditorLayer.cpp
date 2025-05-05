#include "pch.h"
#include "EditorLayer.h"

//#include <imgui/backends/imgui_impl_vulkan.h>

#include "Engine/Core/Core.h"
#include <imgui/imgui.h>

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Engine/Debug/Instrumentor.h>

#include "Engine/Scene/SceneSerializer.h"
#include "Engine/Utils/PlatformUtils.h"

#include "ImGuizmo/ImGuizmo.h"
#include <glm/gtx/string_cast.hpp>
#include "Engine/Math/Math.h"

#include "Sandbox2D.h"

#include "EditorApp.h"
#include <Engine/AssetManager/AssetManager.h>

#include "Engine/Debug/DebugUtils.h"
//#include <imgui/backends/imgui_impl_vulkan.h>
#include "Engine/Renderer/Renderer.h"


 

namespace Engine {

    //temporary
    //extern const std::filesystem::path s_assetPath;


    static const uint32_t s_mapWidth = 26;
    static const char* s_mapTiles =
        "WWWWWWWWWWWWWWWWWWWWWWWWWWW"
        "WWWWWWDDDDWWWWWWWWWWWWWWWWW"
        "WWWWDDDDDDDDDWWWWWWWWWWWWWW"
        "WWWDDDDDDDDDDDDWWWWWWWWWWWW"
        "WWDDDDDDDDDDDDDDWWWWWWWWWWW"
        "WWDDDDDDDDDDDDDDDDWWWWWWWWW"
        "WWWDDDDDDDDDDDDDWWWWWWWWWWW"
        "WWWWWWWDDWWWWWWWWWWWWWWWWWW"
        "WWWWDDDDDDDDDDDWWWWWWWWWWWW"
        "WWWWWDDDDDDDDWWWWWWWWWWWWWW"
        "WWWWWWWWWWWWWWWWWWWWWWWWWWW"
        "WWWWWCWWWWWWWWWWWWWWWWWWWWW"
        ;

    class Editor;

    EditorLayer::EditorLayer(Editor* editor)
        : Layer("EditorLayer"),
        m_orthoCameraController(1280.0f / 720.0f, true),
        m_editor(editor)
    {

    }

    void EditorLayer::OnAttach()
    {
        EE_PROFILE_FUNCTION();



	    //m_iconStop = AssetManager::AddTexture("stopButton", AssetManager::GetAssetPath("icons/stop-button.png").string());
        m_iconPlay = std::make_shared<VulkanTexture>(AssetManager::GetAssetPath("icons/play-button-arrowhead.png").string(), true);
        m_iconStop = std::make_shared<VulkanTexture>(AssetManager::GetAssetPath("icons/stop-button.png").string(), true);
        m_iconPause = std::make_shared<VulkanTexture>(AssetManager::GetAssetPath("icons/video-pause-button.png").string(), true);
		//m_iconPause = AssetManager::AddTexture("pauseButton", AssetManager::GetAssetPath("icons/video-pause-button.png").string());
       
        m_mapWidth = s_mapWidth;
        m_mapHeight = strlen(s_mapTiles) / s_mapWidth;
        //m_textureMap['D'] = Engine::SubTexture2D::CreateFromCoordinates(m_textureSpriteSheetPacked, { 6, 11 }, { 128,128 });
        //m_textureMap['W'] = Engine::SubTexture2D::CreateFromCoordinates(m_textureSpriteSheetPacked, { 11, 11 }, { 128,128 });

        //m_textureBarrel = Engine::SubTexture2D::CreateFromCoordinates(m_textureSpriteSheetPacked, { 8, 0 }, { 128,128 });


        //m_orthoCameraController.SetZoomLevel(10.0f);

        Engine::FramebufferSpecification framebufferSpecs;

        framebufferSpecs.Attachments = { FramebufferTextureFormat::RGBA8,FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
        framebufferSpecs.Height = 720;
        framebufferSpecs.Width = 1280;
        //m_framebuffer = Engine::Framebuffer::Create(framebufferSpecs);

        m_editorCamera = EditorCamera(30.0f, 1.78f, 0.1f, 1000.0f);

       // m_framebuffer = m_editor.get()->GetGameLayer()->GetGameFramebuffer();

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

        m_activeScene = std::make_shared<Scene>();
        //m_activeScene = m_editor.get()->GetGameLayer()->GetActiveGameScene();


        m_editorScene = std::make_shared<Scene>();
        //m_activeScene = Scene::Copy(m_editor.get()->GetGameLayer()->GetActiveGameScene());
        //m_editorScene->OnRunTimeStart();
        //m_editorScene = Scene::Copy(m_editor.get()->GetGameLayer()->GetActiveGameScene());

        
        
        
        Engine::SceneSerializer serializer(m_editorScene);
        std::string scenePath = AssetManager::GetScenePath(m_editor.get()->GetGameLayer()->GetActiveSceneName()).string();
        if (!serializer.Deserialize(scenePath))
        {
            EE_CORE_ERROR("Failed to load scene at: {}", scenePath);
        }
        
        
        
        
       // m_editorScene->OnViewportResize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);
        //m_activeScene = Scene::Copy(m_editorScene);
        m_sceneHierarchyPanel.SetEditorContext(m_editorScene);
        m_sceneHierarchyPanel.SetGameContext(m_editor.get()->GetGameLayer()->GetActiveGameScene());
        m_sceneHierarchyPanel.SetNewComponentsContext(m_editorScene); // remove this and only use new components?

       //m_sceneHierarchyPanel.SetContext(Scene::Combine(m_editorScene, m_editor.get()->GetGameLayer()->GetActiveGameScene()));
        //m_sceneHierarchyPanel.SetContext(m_editor.get()->GetGameLayer()->GetActiveGameScene());
      
        //m_editor.get()->GetGameLayer()->SetActiveScene(Scene::Combine(m_editorScene, m_editor.get()->GetGameLayer()->GetActiveGameScene()));
        m_currentScenePath = AssetManager::GetScenePath(m_editor.get()->GetGameLayer()->GetActiveSceneName());
        //m_editor.get()->GetGameLayer()->GetActiveGameScene()->OnRunTimeStart();

    }

    void EditorLayer::OnDetach()
    {
        EE_PROFILE_FUNCTION();
        //m_sceneSerializer->Serialize("assets/scene/example_scene.ee");
        
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
                    Engine::Application::Get().Close();
                }

                ImGui::Separator();


                ImGui::EndMenu();
            }


            m_sceneHierarchyPanel.OnImGuiRender();

            m_contentBrowserPanel.OnImGuiRender();

            ImGui::Begin("Stats");
            auto stats = Engine::VulkanRenderer2D::GetStats();
            ImGui::Text("Renderer2D Stats:");
            ImGui::Text("Draw Calls: %d", stats.DrawCalls);
            ImGui::Text("Quads: %d", stats.QuadCount);
            ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
            ImGui::Text("Indicies: %d", stats.GetTotalIndexCount());
            ImGui::Text("Lines: %d", stats.LineCount);
            ImGui::Text("Texture GPU memory cache: %.2f MB", AssetManager::s_totalTextureMemory / (1024.0f * 1024.0f));
            ImGui::Text("FPS: %d", m_fpsCounter.GetFPS());

            ImGui::End();



            ImGui::Begin("Settings");
            ImGui::Checkbox("Show colliders", &m_showColliders);
            ImGui::End();

            //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });

            ImGui::Begin("Viewport");
            ImVec2 viewportOffset = ImGui::GetCursorPos();


            m_viewportFocused = ImGui::IsWindowFocused();
            m_viewportHovered = ImGui::IsWindowHovered();
            Application::Get().GetImGuiLayer()->BlockEvents(!m_viewportFocused && !m_viewportHovered);


            ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
            m_viewportSize = { viewportPanelSize.x, viewportPanelSize.y };

            
            if (m_editor)
            {
                // Ensure that GetColorAttachmentRendererID() is valid
                ImTextureID textureID = (ImTextureID)Renderer::GetCurrentGameDescriptorSet();
                if (textureID != 0)
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
                    
                    ImGui::Image(textureID, ImVec2{ m_viewportSize.x, m_viewportSize.y }, ImVec2{ 0,1 }, ImVec2{ 1, 0 });
                }
            }
            

            ImVec2 windowSize = ImGui::GetWindowSize();
            ImVec2 minBound = ImGui::GetWindowPos();
            minBound.x += viewportOffset.x;
            minBound.y += viewportOffset.y;

            ImVec2 maxBound = { minBound.x + viewportPanelSize.x, minBound.y + viewportPanelSize.y };
            m_viewportBounds[0] = { minBound.x ,minBound.y };
            m_viewportBounds[1] = { maxBound.x ,maxBound.y };

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

            if (selectedEntity && selectedEntity.HasComponent<TransformComponent>() &&
                  m_sceneHierarchyPanel.GetGuizmoType() != -1)
            {
                
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
                glm::mat4 cameraView = m_editorCamera.GetViewMatrix();


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

        ImGui::End();

    }

    void EditorLayer::UI_Toolbar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });

        ImGui::Begin("##toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        float size = ImGui::GetWindowHeight() - 10.0f;
        Ref<VulkanTexture> icon = m_sceneState == SceneState::Play ?  m_iconPause : m_iconPlay;

        // Centering the toolbar buttons
        float toolbarWidth = 2 * size;  // Adjust based on number of buttons
        float offsetX = (ImGui::GetWindowContentRegionMax().x - toolbarWidth) * 0.5f;
        ImGui::SetCursorPosX(offsetX);

        // Play Button
        if (ImGui::ImageButton("##playbutton", (ImTextureID)icon->GetTextureDescriptor(), ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1)))
        {
            if (m_sceneState == SceneState::Pause || m_sceneState == SceneState::Edit)
            {
                OnScenePlay();
            }
            else if (m_sceneState == SceneState::Play)
            {
                OnScenePause();
            }
        }

        ImGui::SameLine(); // Move the next item to the same row

        // Stop Button
        if (ImGui::ImageButton("##stopbutton", (ImTextureID)m_iconStop->GetTextureDescriptor(), ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1)))
        {
            if (m_sceneState == SceneState::Play || m_sceneState == SceneState::Pause)
            {
                OnSceneStop();
            }
        }

        ImGui::PopStyleColor(1);
        ImGui::PopStyleVar(2);
        ImGui::End();
    }


    void EditorLayer::OnScenePlay()
    {

        m_editor.get()->GetGameLayer()->SetActiveScene(Scene::Combine(m_sceneHierarchyPanel.GetNewComponentsContext(), m_editor.get()->GetGameLayer()->GetActiveGameScene()));
       // m_editor.get()->GetGameLayer()->GetActiveGameScene()->OnRunTimeStart();
        
        if (m_sceneState != SceneState::Pause)
        {
            m_sceneHierarchyPanel.SetGameContext(m_editor.get()->GetGameLayer()->GetActiveGameScene());
            m_editor.get()->GetGameLayer()->OnGameStart();
        }

        m_sceneState = SceneState::Play;

       // m_editor.get()->GetGameLayer()->SetIsPlaying(true);
         
    }

    void EditorLayer::OnSceneStop()
    {      
        m_sceneState = SceneState::Edit;
        //m_activeScene = m_editorScene;

        m_editor.get()->GetGameLayer()->SetIsPlaying(false);
        m_editor.get()->GetGameLayer()->GetActiveGameScene()->OnRunTimeStop();
        m_editor.get()->GetGameLayer()->OnGameStop();
        //m_editor.get()->GetGameLayer()->SetActiveScene(m_activeScene);

    }

    void EditorLayer::OnScenePause()
    {
        m_sceneState = SceneState::Pause;
        //m_activeScene->OnRunTimeStop();


        m_editor.get()->GetGameLayer()->SetIsPlaying(false);
    }

    void EditorLayer::OnDuplicateEntity()
    {
        if (m_sceneState != SceneState::Edit)
        {
            return;
        }

        Entity selectedEntity = m_sceneHierarchyPanel.GetSelectedEntity();
        if (selectedEntity)
        {
            m_editorScene->DuplicateEntity(selectedEntity);
        }
    }

    void EditorLayer::OnOverlayRender()
    {
        if (m_sceneState == SceneState::Play)
        {
            Entity camera = m_editor.get()->GetGameLayer()->GetActiveGameScene()->GetPrimaryCameraEntity();
            if (!camera)
            {
                return;
            }

            Renderer2D::BeginScene(camera.GetComponent<CameraComponent>().Camera, camera.GetComponent<TransformComponent>().GetTransform());
        }
        else
        {

            Renderer2D::BeginScene(m_editorCamera);
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



                    Renderer2D::DrawCircle(transform, glm::vec4(0.0f, 0.9f, 0.0f, 1.0f), 0.1f);
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

                    Renderer2D::DrawRect(transform, glm::vec4(0.0f, 0.9f, 0.0f, 1.0f), 0.1f);
                }

            }
        }
       
        Renderer2D::EndScene();
    }


    void EditorLayer::OnUpdate(Engine::Timestep timestep)
    {
        EE_PROFILE_FUNCTION();

        //Renderer::DrawFrame();
        

        /*
        FramebufferSpecification spec = m_framebuffer->GetSpecification();
        if (m_viewportSize.x > 0.0f && m_viewportSize.y > 0.0f &&
            (spec.Width != static_cast<uint32_t>(m_viewportSize.x) ||
                spec.Height != static_cast<uint32_t>(m_viewportSize.y)))
        {
            m_framebuffer->Resize(static_cast<uint32_t>(m_viewportSize.x), static_cast<uint32_t>(m_viewportSize.y));
            m_orthoCameraController.OnResize(static_cast<uint32_t>(m_viewportSize.x), static_cast<uint32_t>(m_viewportSize.y));
            m_editorCamera.SetViewportSize(m_viewportSize.x, m_viewportSize.y);
            m_editor.get()->GetGameLayer()->GetActiveGameScene()->OnViewportResize(static_cast<uint32_t>(m_viewportSize.x), static_cast<uint32_t>(m_viewportSize.y));
        }

        */
        if (m_viewportFocused)
        {
            m_orthoCameraController.OnUpdate(timestep);
        }

        m_editorCamera.OnUpdate(timestep);

        m_fpsCounter.Update(timestep);

        // ******** Render ***********

         //statistics
        //Engine::VulkanRenderer2D::ResetStats();
        {
            EE_PROFILE_SCOPE("render pre");
           // m_framebuffer->Bind();
            //Engine::RenderCommand::SetClearColor({ 0.2f, 0, 0.2f, 1 });
            //Engine::RenderCommand::Clear();
        }

       // m_framebuffer->ClearColorAttachment(1, -1);

        {
            EE_PROFILE_SCOPE("render draw");
            //********* update scene *********

            switch (m_sceneState)   
            {
                case Engine::EditorLayer::SceneState::Edit:
                {

                    m_editor.get()->GetGameLayer()->GetActiveGameScene()->OnUpdateEditor(timestep, m_editorCamera);
                    break;
                }
                case Engine::EditorLayer::SceneState::Play:
                {
                    m_editor.get()->GetGameLayer()->GetActiveGameScene()->OnUpdateRuntime(timestep);
                    break;
                }
                case Engine::EditorLayer::SceneState::Pause:
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

           //EE_CORE_INFO("mouseX: {0}, {1}", mouseX, mouseY);
           //EE_CORE_INFO("viewportSize: {0}, {1}", viewportSize.x, viewportSize.y);

            /*
            m_mouseIsInViewPort = mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y;
            if (m_mouseIsInViewPort)
            {
                int pixelData = m_framebuffer->ReadPixel(1, mouseX, mouseY);
                if (pixelData == -1)
                {
                    m_hoveredEntity = {};
                }
                else
                {
                    m_hoveredEntity = Entity{ (entt::entity)pixelData, m_activeScene.get()};

                }
            }
            */

           // OnOverlayRender();

           /// m_framebuffer->Unbind();
        }

        

    }

    void EditorLayer::OnEvent(Engine::Event& event)
    {
        m_orthoCameraController.OnEvent(event);
        m_editorCamera.OnEvent(event);

        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<KeyPressedEvent>(EE_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
        dispatcher.Dispatch<MouseButtonPressedEvent>(EE_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));

    }

    bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
    {
        if (e.GetMouseButton() == Mouse::Button0)
        {
           
            if (m_mouseIsInViewPort && m_hoveredEntity && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt))
            {
                m_sceneHierarchyPanel.SetSelectedEntity(m_hoveredEntity);
                return true;
            }
            else if(m_mouseIsInViewPort && !ImGuizmo::IsOver() && !m_hoveredEntity)
            {
                // reset selected entity
                //m_sceneHierarchyPanel.SetSelectedEntity({}); 
                //return true;
            }
        }
        return false;
    }

    bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
    {
        // shortcuts
        if (e.IsRepeat())
        {
            return false;
        }

        bool shiftPressed = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);
        bool controlPressed = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);

        switch (e.GetKeyCode())
        {
        case Key::N:
        {
            if (controlPressed)
            {
                NewScene();
            }
            break;
        }
        case Key::S:
        {
            if (controlPressed && shiftPressed)
            {
                SaveSceneAs();
            }
            else if (controlPressed)
            {
                SaveScene();
            }
            break;
        }
        case Key::O:
        {
            if (controlPressed)
            {
                OpenScene();
            }
            break;
        }
        case Key::D:
        {
            if (controlPressed)
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
        }
        return false;
    }

    

    void EditorLayer::NewScene()
    {
        m_activeScene = std::make_shared<Scene>();
        m_activeScene->OnViewportResize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);
        m_sceneHierarchyPanel.SetEditorContext(m_activeScene);
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

    void EditorLayer::OpenScene(const std::filesystem::path& path)
    {
        if (m_sceneState != SceneState::Edit)
        {
            OnSceneStop();
        }

        if (path.extension().string() != ".ee")
        {
            EE_CORE_WARN("could not load {} - not s scene file .ee", path.filename().string());
            return;
        }

        m_editorScene = std::make_shared<Scene>();
        m_editorScene->OnViewportResize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);
        m_sceneHierarchyPanel.SetEditorContext(m_editorScene);

        SceneSerializer serializer(m_editorScene);
        serializer.Deserialize(path.string());

        m_activeScene = m_editorScene;
        m_currentScenePath = path;
    }

    void EditorLayer::SaveSceneAs()
    {
        std::string filepath = FileDialogs::SaveFile("Engine scene (*.ee)\0*.ee\0");
        if (!filepath.empty())
        {
            SceneSerializer serializer(m_activeScene);
            serializer.Serialize(filepath);
            m_currentScenePath = filepath;
        }
    }



    void EditorLayer::SaveScene()
    {
        if (!m_currentScenePath.empty())
        {

            SceneSerializer serializer(Scene::Combine(m_editorScene, m_sceneHierarchyPanel.GetNewComponentsContext()));

            // Log before saving to check what exists in the scene
            DebugUtils::LogAllEntitiesWithComponents(m_editorScene);
            DebugUtils::LogAllEntitiesWithComponents(m_sceneHierarchyPanel.GetNewComponentsContext());

            // Serialize the current editor scene without reloading it
            serializer.Serialize(m_currentScenePath.string());

        }
    }


  

}