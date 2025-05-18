#pragma once
#include "filesystem"
#include <Engine/Platform/Vulkan/VulkanTexture.h>

namespace Engine {

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();
		void OnImGuiRender();


	private:
		std::filesystem::path m_currentDirectory;
		Ref<VulkanTexture> m_folderIconTexture;
		Ref<VulkanTexture> m_fileIconTexture;

	};

}
