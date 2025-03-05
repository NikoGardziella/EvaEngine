#include "pch.h"
#include "EditorLayer.h"
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


namespace Engine {

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

    EditorLayer::EditorLayer()
        : Layer("EditorLayer"),
        m_orthoCameraController(1280.0f / 720.0f, true)
    {

    }

    void EditorLayer::OnAttach()
    {
        EE_PROFILE_FUNCTION();

        m_checkerBoardTexture = Engine::Texture2D::Create("assets/textures/chess_board.png");
        m_textureSpriteSheetPacked = Engine::Texture2D::Create("assets/textures/game/RPGpack_sheet_2X.png");

        m_mapWidth = s_mapWidth;
        m_mapHeight = strlen(s_mapTiles) / s_mapWidth;
        m_textureMap['D'] = Engine::SubTexture2D::CreateFromCoordinates(m_textureSpriteSheetPacked, { 6, 11 }, { 128,128 });
        m_textureMap['W'] = Engine::SubTexture2D::CreateFromCoordinates(m_textureSpriteSheetPacked, { 11, 11 }, { 128,128 });

        m_textureBarrel = Engine::SubTexture2D::CreateFromCoordinates(m_textureSpriteSheetPacked, { 8, 0 }, { 128,128 });


        //m_orthoCameraController.SetZoomLevel(10.0f);

        Engine::FramebufferSpecification framebufferSpecs;

        framebufferSpecs.Attachments = { FramebufferTextureFormat::RGBA8,FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
        framebufferSpecs.Height = 720;
        framebufferSpecs.Width = 1280;
        m_framebuffer = Engine::Framebuffer::Create(framebufferSpecs);

        m_activeScene = std::make_shared<Scene>();
        
        m_editorCamera = EditorCamera(30.0f, 1.78f, 0.1f, 1000.0f);


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



        m_sceneHierarchyPanel.SetContext(m_activeScene);

    }

    void EditorLayer::OnDetach()
    {
        EE_PROFILE_FUNCTION();
        //m_sceneSerializer->Serialize("assets/scene/example_scene.ee");
    }

    void EditorLayer::OnImGuiRender()
    {
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

                ImGui::Separator();

                if (ImGui::MenuItem("Exit"))
                {
                    Engine::Application::Get().Close();
                }

                ImGui::Separator();


                ImGui::EndMenu();
            }


            m_sceneHierarchyPanel.OnImGuiRender();

            ImGui::Begin("Settings");

            std::string name = "None";
            if (m_hoveredEntity)
            {
                name = m_hoveredEntity.GetComponent<TagComponent>().Tag;
            }
            ImGui::Text("Entity: %s", name.c_str());


            auto stats = Engine::Renderer2D::GetStats();
            ImGui::Text("Renderer2D Stats:");
            ImGui::Text("Draw Calls: %d", stats.DrawCalls);
            ImGui::Text("Quads: %d", stats.QuadCount);
            ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
            ImGui::Text("Indicies: %d", stats.GetTotalIndexCount());



            ImGui::End();


            //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });

            ImGui::Begin("Viewport");
            ImVec2 viewportOffset = ImGui::GetCursorPos();


            m_viewportFocused = ImGui::IsWindowFocused();
            m_viewportHovered = ImGui::IsWindowHovered();
            Application::Get().GetImGuiLayer()->BlockEvents(!m_viewportFocused && !m_viewportHovered);

            ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
            m_viewportSize = { viewportPanelSize.x, viewportPanelSize.y };

            uint32_t textureID = m_framebuffer->GetColorAttachmentRendererID();
            ImGui::Image(textureID, ImVec2{ m_viewportSize.x, m_viewportSize.y }, ImVec2{ 0,1 }, ImVec2{ 1, 0 });


            ImVec2 windowSize = ImGui::GetWindowSize();
            ImVec2 minBound = ImGui::GetWindowPos();
            minBound.x += viewportOffset.x;
            minBound.y += viewportOffset.y;

            ImVec2 maxBound = { minBound.x + viewportPanelSize.x, minBound.y + viewportPanelSize.y };
            m_viewportBounds[0] = { minBound.x ,minBound.y };
            m_viewportBounds[1] = { maxBound.x ,maxBound.y };

            // Guizmo
           
            Entity selectedEntity = m_sceneHierarchyPanel.GetSelectedEntity();
            auto cameraEntity = m_activeScene->GetPrimaryCameraEntity();

            if (selectedEntity && selectedEntity.HasComponent<TransformComponent>() &&
                cameraEntity && m_sceneHierarchyPanel.GetGuizmoType() != -1)
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


        ImGui::End();

    }

    void EditorLayer::OnUpdate(Engine::Timestep timestep)
    {
        EE_PROFILE_FUNCTION();

        FramebufferSpecification spec = m_framebuffer->GetSpecification();
        if (m_viewportSize.x > 0.0f && m_viewportSize.y > 0.0f &&
            (spec.Width != static_cast<uint32_t>(m_viewportSize.x) ||
                spec.Height != static_cast<uint32_t>(m_viewportSize.y)))
        {
            m_framebuffer->Resize(static_cast<uint32_t>(m_viewportSize.x), static_cast<uint32_t>(m_viewportSize.y));
            m_orthoCameraController.OnResize(static_cast<uint32_t>(m_viewportSize.x), static_cast<uint32_t>(m_viewportSize.y));
            m_editorCamera.SetViewportSize(m_viewportSize.x, m_viewportSize.y);
            m_activeScene->OnViewportResize(static_cast<uint32_t>(m_viewportSize.x), static_cast<uint32_t>(m_viewportSize.y));
        }

        if (m_viewportFocused)
        {
            m_orthoCameraController.OnUpdate(timestep);
        }

        m_editorCamera.OnUpdate(timestep);

        // ******** Render ***********

         //statistics
        Engine::Renderer2D::ResetStats();
        {
            EE_PROFILE_SCOPE("render pre");
            m_framebuffer->Bind();
            Engine::RenderCommand::SetClearColor({ 0.2f, 0, 0.2f, 1 });
            Engine::RenderCommand::Clear();
        }

        m_framebuffer->ClearColorAttachment(1, -1);

        {
            EE_PROFILE_SCOPE("render draw");
            //********* update scene *********
            m_activeScene->OnUpdateEditor(timestep, m_editorCamera);


            ImVec2 mousePos = ImGui::GetMousePos();
            mousePos.x -= m_viewportBounds[0].x;
            mousePos.y -= m_viewportBounds[0].y;
            glm::vec2 viewportSize = m_viewportBounds[1] - m_viewportBounds[0];

            mousePos.y = viewportSize.y - mousePos.y;
            int mouseX = (int)mousePos.x;
            int mouseY = (int)mousePos.y;

           EE_CORE_INFO("mouseX: {0}, {1}", mouseX, mouseY);
           //EE_CORE_INFO("viewportSize: {0}, {1}", viewportSize.x, viewportSize.y);

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




            m_framebuffer->Unbind();
        }

        /*
        Engine::Renderer2D::BeginScene(m_orthoCameraController.GetCamera());


        for (uint32_t y = 0; y  < m_mapHeight; y ++)
        {
            // TOOD ? : combin the vertices and draw as one mesh
            for (uint32_t x = 0; x < m_mapWidth; x++)
            {
                char tileType = s_mapTiles[x + y * m_mapWidth];
                Engine::Ref<Engine::SubTexture2D> texture;
                if (m_textureMap.find(tileType) != m_textureMap.end())
                {
                    texture = m_textureMap[tileType];
                }
                else
                {
                    texture = m_textureBarrel;

                }
                Engine::Renderer2D::DrawQuad({ x - m_mapWidth / 2.0f,m_mapHeight- y - m_mapHeight / 2.0f, 0.1f }, { 1.0f, 1.0f, }, texture);

            }
        }

        Engine::Renderer2D::EndScene();
        */

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
           

            if (m_hoveredEntity && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt))
            {
                m_sceneHierarchyPanel.SetSelectedEntity(m_hoveredEntity);
                return true;
            }
            else if(m_mouseIsInViewPort && !ImGuizmo::IsOver() && !m_hoveredEntity)
            {
                // reset selected entity
                m_sceneHierarchyPanel.SetSelectedEntity({}); 
                return true;
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
        m_sceneHierarchyPanel.SetContext(m_activeScene);
    }

    void EditorLayer::OpenScene()
    {
        std::string filepath = FileDialogs::OpenFile("Engine scene (*.ee)\0*.ee\0");
        if (!filepath.empty())
        {
            m_activeScene = std::make_shared<Scene>();
            m_activeScene->OnViewportResize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);
            m_sceneHierarchyPanel.SetContext(m_activeScene);

            SceneSerializer serializer(m_activeScene);
            serializer.Deserialize(filepath);
        }
    }

    void EditorLayer::SaveSceneAs()
    {
        std::string filepath = FileDialogs::SaveFile("Engine scene (*.ee)\0*.ee\0");
        if (!filepath.empty())
        {
            SceneSerializer serializer(m_activeScene);
            serializer.Serialize(filepath);
        }
    }

}