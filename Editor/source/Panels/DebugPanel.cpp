#include "pch.h"

#include "DebugPanel.h"
#include "Engine/Debug/DebugInterface.h"
#include "imgui/imgui.h"
#include <imgui/imgui_internal.h>
#include <Engine/Renderer/VulkanRenderer2D.h>
#include <Engine/Scene/Component.h>
#include "Engine/Scene/Entity.h"

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
                DebugInterface::ResetAllTextures(m_gameContext->GetRegistry());
            }


            ImGui::Checkbox("Show Chunks", &m_showChunks);

            if (boldFont)
                ImGui::PopFont();

            ImGui::TreePop();
        }

		if (m_showChunks)
		{	
            
            Entity camera = m_gameContext->GetPrimaryCameraEntity();
            if (camera)
            {
                Engine::VulkanRenderer2D::BeginScene(camera.GetComponent<CameraComponent>().Camera.GetViewProjection(), camera.GetComponent<TransformComponent>().GetTransform());

            }
            else
            {
               // Engine::VulkanRenderer2D::BeginScene(m_editorCamera);

            }

            
            

            DebugInterface::DebugDrawChunkOutlines(m_gameContext->GetRegistry());
            Engine::VulkanRenderer2D::EndScene();

		}

        

        ImGui::End();
    }
    void DebugPanel::SetGameContext(const Ref<Scene>& scene)
    {
		m_gameContext = scene;
		
    }

}