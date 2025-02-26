#include "pch.h"
#include "EditorLayer.h"
#include "Engine/Core/Core.h"
#include <imgui/imgui.h>

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Engine/Debug/Instrumentor.h>

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


        m_orthoCameraController.SetZoomLevel(10.0f);

        Engine::FrameBufferSpecification framebufferSpecs;

        framebufferSpecs.Height = 720;
        framebufferSpecs.Width = 1280;
        m_framebuffer = Engine::Framebuffer::Create(framebufferSpecs);


        m_activeScene = std::make_shared<Scene>();


        m_squareEntity = m_activeScene->CreateEntity("square");

       //m_squareEntity.HasComponent<TransformComponent>();

        m_squareEntity.AddComponent<TransformComponent>();
        m_squareEntity.AddComponent<SpriteRendererComponent>(glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f});
       // m_squareEntity = square;

       m_cameraEntity = m_activeScene->CreateEntity("camera entity");
       m_cameraEntity.AddComponent<TransformComponent>();
       auto& cameraComp = m_cameraEntity.AddComponent<CameraComponent>();
       cameraComp.FixedAspectRatio = true;

       m_cameraSecondaryEntity = m_activeScene->CreateEntity("secondary camera entity");
       m_cameraSecondaryEntity.AddComponent<TransformComponent>();
       auto& cameraSecondaryComp = m_cameraSecondaryEntity.AddComponent<CameraComponent>();
       cameraSecondaryComp.Primary = false;


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

       m_cameraSecondaryEntity.AddComponent<NativeScriptComponent>().Bind<CameraController>();


       m_sceneHierarchyPanel.SetContext(m_activeScene);

    }

    void EditorLayer::OnDetach()
    {
        EE_PROFILE_FUNCTION();

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
                ImGui::MenuItem("Padding", NULL, &opt_padding);
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

            auto stats = Engine::Renderer2D::GetStats();
            ImGui::Text("Renderer2D Stats:");
            ImGui::Text("Draw Calls: %d", stats.DrawCalls);
            ImGui::Text("Quads: %d", stats.QuadCount);
            ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
            ImGui::Text("Indicies: %d", stats.GetTotalIndexCount());

          

            ImGui::End();


            //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
                
            ImGui::Begin("Viewport");

            m_viewportFocused = ImGui::IsWindowFocused();
            m_viewportHovered = ImGui::IsWindowHovered();
            Application::Get().GetImGuiLayer()->BlockEvents(!m_viewportFocused || !m_viewportHovered);
            

            ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
            m_viewportSize = { viewportPanelSize.x, viewportPanelSize.y };

            /*
            ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
            if (m_viewportSize != *(glm::vec2*)&viewportPanelSize) // Memory layout is same. Two floats
            {
                m_viewportSize = {viewportPanelSize.x, viewportPanelSize.y};
                m_framebuffer->Resize((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);

                m_orthoCameraController.OnResize(viewportPanelSize.x, viewportPanelSize.y);
                m_activeScene->OnViewportResize(static_cast<uint32_t>(m_viewportSize.x), static_cast<uint32_t>(m_viewportSize.y));

            }
            */
            uint32_t textureID = m_framebuffer->GetColorAttachmentRendererID();
            ImGui::Image(textureID, ImVec2{ m_viewportSize.x, m_viewportSize.y }, ImVec2{ 0,1 }, ImVec2{1, 0});

            

            ImGui::End();

            ImGui::EndMenuBar();
        }


        ImGui::End();

    }

    void EditorLayer::OnUpdate(Engine::Timestep timestep)
    {
        EE_PROFILE_FUNCTION();

        
        FrameBufferSpecification spec = m_framebuffer->GetSpecification();
        if (m_viewportSize.x > 0.0f && m_viewportSize.y > 0.0f &&
            (spec.Width != static_cast<uint32_t>(m_viewportSize.x) ||
                spec.Height != static_cast<uint32_t>(m_viewportSize.y)))
        {
            m_framebuffer->Resize(static_cast<uint32_t>(m_viewportSize.x), static_cast<uint32_t>(m_viewportSize.y));
            m_orthoCameraController.OnResize(static_cast<uint32_t>(m_viewportSize.x), static_cast<uint32_t>(m_viewportSize.y));

            m_activeScene->OnViewportResize(static_cast<uint32_t>(m_viewportSize.x), static_cast<uint32_t>(m_viewportSize.y));
        }

        

        if (m_viewportFocused)
        {
            m_orthoCameraController.OnUpdate(timestep);
        }


        // ******** Render ***********

         //statistics
        Engine::Renderer2D::ResetStats();
        {
            EE_PROFILE_SCOPE("render pre");
            m_framebuffer->Bind();
            Engine::RenderCommand::SetClearColor({ 0.2f, 0, 0.2f, 1 });
            Engine::RenderCommand::Clear();
        }

        {
            EE_PROFILE_SCOPE("render draw");
            //********* update scene *********
            m_activeScene->OnUpdate(timestep);

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

    }



}
