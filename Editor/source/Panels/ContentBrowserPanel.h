#pragma once
#include "filesystem"
#include <gl/GL.h>


namespace Engine {

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();
		void OnImGuiRender();


	private:
		std::filesystem::path m_currentDirectory;
		GLuint m_folderIconTexture;
		GLuint ContentBrowserPanel::LoadTexture(const std::string& filepath);

	};

}
