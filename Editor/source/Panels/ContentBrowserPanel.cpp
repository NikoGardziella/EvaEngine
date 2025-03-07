#include "pch.h"
#include "ContentBrowserPanel.h"
#include <imgui/imgui.h>
#include "filesystem"

#include <stb_image/stb_image.h>
//#include <GLAD/include/glad/glad.h>



namespace Engine {

	static const std::filesystem::path s_assetPath = "assets";


	ContentBrowserPanel::ContentBrowserPanel()
		: m_currentDirectory(s_assetPath)
	{
        m_folderIconTexture = LoadTexture("assets/icons/folder_6458782.png");
	}

    void ContentBrowserPanel::OnImGuiRender()
    {
        ImGui::Begin("Content Browser");

        // Back Button
        if (m_currentDirectory != std::filesystem::path(s_assetPath))
        {
            if (ImGui::Button("<-"))
            {
                m_currentDirectory = m_currentDirectory.parent_path();
            }
        }

        ImGui::Separator(); // Visual separation
        ImGui::Columns(4, nullptr, false); // 4-column grid layout (adjust as needed)

        for (auto& p : std::filesystem::directory_iterator(m_currentDirectory))
        {
            std::filesystem::path path = p.path();
            std::filesystem::path relativePath = std::filesystem::relative(path, s_assetPath);
            std::string filename = path.filename().string();

            // Style for buttons
            ImVec2 buttonSize(80, 80); // Uniform button size
            ImGui::PushID(filename.c_str()); // Unique ID for each entry

            if (p.is_directory())
            {
                if (m_folderIconTexture)
                {
                    if (ImGui::ImageButton("##folder", (intptr_t)m_folderIconTexture, ImVec2(32, 32)))
                    {
                        m_currentDirectory /= path.filename();
                    }
                }
                else
                {
                    ImGui::Text("Failed to load texture!");
                }
            }
            else
            {
                // Placeholder for file icon
                if (ImGui::Button(("📄 " + filename).c_str(), buttonSize))
                {
                    // TODO: Handle file selection
                }
            }

            ImGui::TextWrapped("%s", filename.c_str()); // Display filename below button

            ImGui::PopID(); // Restore ImGui state
            ImGui::NextColumn(); // Move to next column
        }

        ImGui::Columns(1); // Reset columns
        ImGui::End();
    }

    GLuint ContentBrowserPanel::LoadTexture(const std::string& filepath)
    {
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        int width, height, channels;
        unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 4);

        if (data)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        else
        {
            std::cerr << "Failed to load texture: " << filepath << std::endl;
        }

        stbi_image_free(data);
        return textureID;
    }

}
