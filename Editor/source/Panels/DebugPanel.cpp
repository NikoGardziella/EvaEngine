#include "pch.h"

#include "DebugPanel.h"
#include "Engine/Debug/DebugInterface.h"
#include "imgui/imgui.h"
#include <imgui/imgui_internal.h>
#include <Engine/Renderer/VulkanRenderer2D.h>
#include <Engine/Scene/Component.h>
#include "Engine/Scene/Entity.h"
#include "../EditorApp.h"
#include "Engine/Map/Grid/GridMap.h"

namespace Engine {
   
    void DebugPanel::OnImGuiRender()
    {
        EE_PROFILE_FUNCTION();


        ImGui::Begin("Debug panel");
        ImGuiIO& io = ImGui::GetIO();

        ImFont* boldFont = io.Fonts->Fonts[0];
        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

        // Optional style if you want to color the button
        // ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.80f, 0.10f, 0.2f, 1.0f });
        // ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.90f, 0.2f, 0.2f, 1.0f });
        // ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.75f, 0.12f, 0.12f, 1.0f });

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

         

        if (ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)1, flags, "Textures:"))
        {
            if (boldFont)
                ImGui::PushFont(boldFont);

            if (ImGui::Button("Unload all textures"))
            {
                DebugInterface::ResetAllTextures(m_gameContext.get());
            }


            ImGui::Checkbox("Show Chunks", &m_showChunks);
            ImGui::Checkbox("Show LOS", &m_showLOS);

            if (boldFont)
                ImGui::PopFont();

            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)2, flags, "Grid:"))
        {
            if (boldFont)
                ImGui::PushFont(boldFont);
   
            ImGui::Checkbox("Show Grid", &m_showGrid);

            if (boldFont)
                ImGui::PopFont();

            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)3, flags, "Combat:"))
        {
            if (boldFont)
                ImGui::PushFont(boldFont);

            ImGui::Checkbox("Show LOS", &m_showLOS);

            if (boldFont)
                ImGui::PopFont();

            ImGui::TreePop();
        }

		if (m_showChunks)
		{	
            
            Entity camera = m_gameContext->GetPrimaryCameraEntity();
            if (camera)
            {
                Engine::VulkanRenderer2D::BeginScene(camera.GetComponent<CameraComponent>().Camera.GetProjection(), camera.GetComponent<TransformComponent>().GetTransform());

            }


            DebugInterface::DebugDrawChunkOutlines(m_gameContext.get());
            Engine::VulkanRenderer2D::EndScene();

		}
        if (m_showGrid)
        {
            Entity camera = m_gameContext->GetPrimaryCameraEntity();
            if (camera)
            {
                Engine::VulkanRenderer2D::BeginScene(camera.GetComponent<CameraComponent>().Camera.GetProjection(), camera.GetComponent<TransformComponent>().GetTransform());
            }
            m_editor->GetGameLayer()->GetActiveGameScene()->GetGrid()->DrawDebugBlockedTiles();


        }

        if (m_showLOS)
        {
            Entity camera = m_gameContext->GetPrimaryCameraEntity();
            if (camera)
            {
                Engine::VulkanRenderer2D::BeginScene(camera.GetComponent<CameraComponent>().Camera.GetProjection(), camera.GetComponent<TransformComponent>().GetTransform());
            }
            m_editor->GetGameLayer()->GetActiveGameScene()->SetDebugDrawLOS(true);

        }
        else
        {
            m_editor->GetGameLayer()->GetActiveGameScene()->SetDebugDrawLOS(false);
        }
        
        int flags3D = static_cast<int>(VulkanRenderer3D::GetDebugFlags());
        if (ImGui::SliderInt("3D render flags", &flags3D, 0, 75))
        {
            VulkanRenderer3D::SetDebugFlags(flags3D);
        }


        ImGui::End();
    }
   

    

    void DebugPanel::SetGameContext(const Ref<Scene>& scene)
    {
		m_gameContext = scene;
		
    }

    void DebugPanel::SetEditor(const Ref<Editor>& editor)
    {
        m_editor = editor;
    }

}