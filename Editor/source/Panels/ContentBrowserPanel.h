#pragma once
#include "filesystem"
#include <gl/GL.h>
#include <Engine/Platform/OpenGl/OpenGLTexture.h>


namespace Engine {

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();
		void OnImGuiRender();


	private:
		std::filesystem::path m_currentDirectory;
		Ref<Texture2D> m_folderIconTexture;
		Ref<Texture2D> m_fileIconTexture;

	};

}
