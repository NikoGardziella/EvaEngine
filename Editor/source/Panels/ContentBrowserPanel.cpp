#include "pch.h"
#include "ContentBrowserPanel.h"
#include "Engine/Core/Log.h"
#include "Engine/AssetManager/AssetManager.h"

#include <imgui/imgui.h>
#include <stb_image/stb_image.h>
//#include <GLAD/include/glad/glad.h>



namespace Engine {

    //extern const std::filesystem::path s_assetPath = AssetManager::GetAssetPath("");


	ContentBrowserPanel::ContentBrowserPanel()
		: m_currentDirectory(AssetManager::GetAssetFolderPath())
	{
        m_folderIconTexture = Engine::Texture2D::Create(AssetManager::GetAssetPath("icons/folder_6458782.png").string());
        m_fileIconTexture = Engine::Texture2D::Create(AssetManager::GetAssetPath("icons/8725956_file_alt_icon.png").string());
	}

    void ContentBrowserPanel::OnImGuiRender()
    {
        ImGui::Begin("Content Browser");

        if (!std::filesystem::exists(AssetManager::GetAssetFolderPath()))
        {
            EE_CORE_ERROR("Assets directory not found: {0}", AssetManager::GetAssetFolderPath().string());
        }

        // Back Button
        if (m_currentDirectory != std::filesystem::path(AssetManager::GetAssetFolderPath()))
        {
            if (ImGui::Button("<-"))
            {
                m_currentDirectory = m_currentDirectory.parent_path();
            }
        }

        ImGui::Separator(); // Visual separation


        ImGui::Columns(5, nullptr, false); // 5 columns, disable border
        try {
        for (auto& p : std::filesystem::directory_iterator(m_currentDirectory))
        {
            std::filesystem::path path = p.path();
            std::filesystem::path relativePath = std::filesystem::relative(path, AssetManager::GetAssetFolderPath());
            std::string filename = path.filename().string();

            ImGui::PushID(filename.c_str());

            if (p.is_directory())
            {
                if (m_folderIconTexture)
                {
                    if (ImGui::ImageButton("##folder", m_folderIconTexture->GetRendererID(), ImVec2(32, 32)))
                    {
                        m_currentDirectory /= path.filename();
                    }
                }
                else
                {
                    ImGui::Text("No Icon");
                }
            }
            else
            {
                if (ImGui::ImageButton("##file", m_fileIconTexture->GetRendererID(), ImVec2(32, 32)))
                {
                    // TODO: Handle file selection
                }
            }

            // Drag-drop functionality
            static std::wstring itemPathW;
            itemPathW = relativePath.wstring();

            if (ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPathW.c_str(),
                    (itemPathW.size() + 1) * sizeof(wchar_t), ImGuiCond_Once);

                ImGui::TextUnformatted(filename.c_str());
                ImGui::EndDragDropSource();
            }

            ImGui::TextWrapped("%s", filename.c_str()); // Display filename under icon

            ImGui::NextColumn(); // Move to next column

            ImGui::PopID();
        
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        EE_CORE_ERROR("Filesystem error: {0}, ", e.what());
    }
        ImGui::Columns(1); // Reset columns
        ImGui::End();
    }

    

}
