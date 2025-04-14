#include "pch.h"
#include "ImGuiLayer.h"
#include <backends/imgui_impl_vulkan.h>


//#include "imgui.h"
//#include <imgui_internal.h>

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>

#include "Engine/Renderer/Renderer.h"
#include "Engine/Core/Application.h"
#include "ImGuizmo.h"
#include <Engine/AssetManager/AssetManager.h>


//remove
#include "GLFW/glfw3.h"



namespace Engine {

	ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer")
	{

	}

	ImGuiLayer::~ImGuiLayer()
	{

	}

	



	void ImGuiLayer::OnAttach()
	{
		EE_PROFILE_FUNCTION();

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;
	//	ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f); // Dark background for ImGui windows
		float fontSize = 18.0f;// *2.0f;
		//io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Bold.ttf", fontSize);
		//io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Regular.ttf", fontSize);

		// Setup Dear ImGui style
		//ImGui::StyleColorsDark();
		//StyleColorsEva();
		//ImGui::StyleColorsClassic();
		//ImGui::StyleColorsLight();
		
		//StyleColorsCyberpunk();
		//StyleColorsMidnightOcean();
		//StyleColorsRustGold();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}


		{
			std::string fontPath = AssetManager::GetAssetPath("fonts/kanit/Kanit-Bold.ttf").string();
			const char* ImguiFont = fontPath.c_str();

			std::ifstream file(ImguiFont);
			if (file.good())
			{
				io.Fonts->AddFontFromFileTTF(ImguiFont, 18.0f);
			}
			else
			{
				EE_CORE_WARN("Failed to load ImGui font: {}", ImguiFont);
			}
		}

		
		{
			std::string defaultfontPath = AssetManager::GetAssetPath("fonts/kanit/Kanit-Regular.ttf").string();
			const char* defaultImguiFont = defaultfontPath.c_str();
			std::ifstream file(defaultImguiFont);
			if (file.good())
			{
				io.Fonts->AddFontFromFileTTF(defaultImguiFont, 18.0f);
			}
			else
			{
				EE_CORE_WARN("Failed to load ImGui font: {}", defaultImguiFont);
			}
		}
		


		//SetDarkThemeColors();
		//StyleColorsEva();

		Application& app = Application::Get();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:
			EE_CORE_ASSERT(false, "RenderAPI not supported");
			return;

		case RendererAPI::API::OpenGL:
			ImGui_ImplGlfw_InitForOpenGL(window, true);
			ImGui_ImplOpenGL3_Init("#version 410");

		case RendererAPI::API::Vulkan:
		{
			 
			ImGui_ImplGlfw_InitForVulkan(window, true);
			VulkanContext* vulkanContext = VulkanContext::Get();

			ImGui_ImplVulkan_InitInfo init_info = {};
			init_info.Instance = vulkanContext->GetVulkanInstance().GetInstance();
			init_info.PhysicalDevice = vulkanContext->GetDeviceManager().GetPhysicalDevice();
			init_info.Device = vulkanContext->GetDeviceManager().GetDevice();
			init_info.Queue = vulkanContext->GetGraphicsQueue();
			init_info.DescriptorPool = vulkanContext->GetImGuiDescriptorPool();
			init_info.ImageCount = vulkanContext->GetVulkanSwapchain().GetSwapchainImages().size();
			init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
			init_info.Subpass = 0;
			init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
			init_info.RenderPass = vulkanContext->GetImGuiRenderPass();
			init_info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
			init_info.ImageCount = MAX_FRAMES_IN_FLIGHT;
			init_info.QueueFamily = vulkanContext->GetDeviceManager().GetGraphicsQueueFamilyIndex();

			if (!ImGui_ImplVulkan_Init(&init_info))
			{
				EE_CORE_ASSERT(false, "Failed to initialize ImGui Vulkan");
			}
			else
			{
				EE_CORE_INFO("ImGui Vulkan initialized successfully");
			}


			
			if (!ImGui_ImplVulkan_CreateFontsTexture())
			{
				EE_CORE_ASSERT(false, "Failed to initialize Fonts for ImGui Vulkan");
			}
			else
			{
				EE_CORE_INFO("ImGui Vulkan Fonts initialized successfully");
			}
			std::vector<Ref<VulkanTexture>> textures = AssetManager::GetAllTextures();

			for (size_t i = 0; i < textures.size(); i++)
			{
				//ImGui_ImplVulkan_AddTexture(textures[i]->GetSampler(), textures[i]->GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			}

			//ImGui_ImplVulkan_DestroyFontsTexture();

		}

			
			//ImGui_ImplVulkan_InitMultiViewportSupport();


		}

	}

	void ImGuiLayer::OnDetach()
	{
		EE_PROFILE_FUNCTION();

		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:
				EE_CORE_ASSERT(false, "RenderAPI not supported");
				return;
			case RendererAPI::API::OpenGL:
			{
				ImGui_ImplOpenGL3_Shutdown();
				ImGui_ImplGlfw_Shutdown();
				ImGui::DestroyContext();
			}
			case RendererAPI::API::Vulkan:
			{
				ImGui_ImplVulkan_Shutdown();
				ImGui_ImplGlfw_Shutdown();
				ImGui::DestroyContext();
			}
		}


	}



	void ImGuiLayer::OnEvent(Event& event)
	{
		if (m_bockImGuiEvents)
		{
			 ImGuiIO& io = ImGui::GetIO();
			 event.Handled |= event.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
			 event.Handled |= event.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
		}
	}


	void ImGuiLayer::Begin()
	{
		

		EE_PROFILE_FUNCTION();
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:
			EE_CORE_ASSERT(false, "RenderAPI not supported");
			return;
		case RendererAPI::API::OpenGL:
		{
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			ImGuizmo::BeginFrame();
			break;
		}
		case RendererAPI::API::Vulkan:
		{
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame(); // or SDL, depending on what you use
			ImGui::NewFrame();

		

			break;
		}

		}
	}

	void ImGuiLayer::End()
	{
		EE_PROFILE_FUNCTION();

		ImGuiIO& io = ImGui::GetIO();
		Application& app = Application::Get();
		io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());

		// Rendering
		ImGui::Render();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		ImDrawData* drawData = ImGui::GetDrawData();
		if (!drawData || drawData->CmdListsCount == 0)
		{
			// No draw data, skip rendering
			return;
		}

		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:
			EE_CORE_ASSERT(false, "RenderAPI not supported");
			return;
		case RendererAPI::API::OpenGL:
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			break;
		case RendererAPI::API::Vulkan:
		{
				/*
			VulkanContext* vulkanContext = VulkanContext::Get();
			uint32_t currentFrame = Renderer::GetCurrentFrame();

			VkCommandBuffer commandBuffer = vulkanContext->GetCommandBuffer(currentFrame);

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			// Begin recording command buffer
			VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
			if (result != VK_SUCCESS) {
				EE_CORE_ASSERT(false, "Failed to begin recording command buffer");
				return;
			}

			// Render pass begin info
			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = vulkanContext->GetImGuiRenderPass();
			renderPassBeginInfo.framebuffer = vulkanContext->GetImGuiFramebuffer(currentFrame);
			renderPassBeginInfo.renderArea = { 0, 0, static_cast<uint32_t>(drawData->DisplaySize.x), static_cast<uint32_t>(drawData->DisplaySize.y) };

			VkClearValue clearValues[2];  // Example, adjust based on your render pass
			clearValues[0].color = { {0.0f, 0.0f, 0.0f, 0.0f} }; // Clear color for first attachment
			clearValues[1].color = { {0.0f, 0.0f, 0.0f, 0.0f} }; // Clear color for second attachment

			renderPassBeginInfo.clearValueCount = 2;  // Set this to the number of clear values
			renderPassBeginInfo.pClearValues = clearValues;
			// Begin the render pass
			vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Record ImGui draw data commands
			ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
			//Renderer::DrawFrame();
			// End the render pass
			vkCmdEndRenderPass(commandBuffer);

			// End the command buffer
			result = vkEndCommandBuffer(commandBuffer);
			if (result != VK_SUCCESS) {
				EE_CORE_ASSERT(false, "Failed to end command buffer");
				return;
			}

			// Submit the command buffer to the Vulkan queue
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;

			result = vkQueueSubmit(vulkanContext->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
			if (result != VK_SUCCESS)
			{
				EE_CORE_ASSERT(false, "Failed to submit command buffer to Vulkan queue");
				return;
			}
				*/

			//Renderer::DrawFrame();

			break;
		}
		}

		
	}


	

	void ImGuiLayer::SetDarkThemeColors()
	{
		auto& colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

		// Headers
		colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Buttons
		colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

		// Title
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	}
	void ImGuiLayer::StyleColorsEva()
	{
		auto& colors = ImGui::GetStyle().Colors;

		// Background
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.0f, 0.15f, 1.0f }; // Deep Purple

		// Headers
		colors[ImGuiCol_Header] = ImVec4{ 0.25f, 0.0f, 0.35f, 1.0f }; // Dark Magenta
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.35f, 0.0f, 0.45f, 1.0f }; // Rich Violet
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.5f, 0.1f, 0.6f, 1.0f }; // Warm Purple

		// Buttons
		colors[ImGuiCol_Button] = ImVec4{ 0.3f, 0.0f, 0.4f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.45f, 0.1f, 0.55f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.6f, 0.2f, 0.7f, 1.0f };

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.0f, 0.3f, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.0f, 0.4f, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.5f, 0.1f, 0.6f, 1.0f };

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4{ 0.2f, 0.0f, 0.25f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.4f, 0.0f, 0.5f, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.5f, 0.1f, 0.6f, 1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.0f, 0.2f, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.3f, 0.0f, 0.35f, 1.0f };

		// Title Bar
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.0f, 0.2f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.25f, 0.0f, 0.35f, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.1f, 0.0f, 0.15f, 1.0f };

		// Resize Grip (for resizable windows)
		colors[ImGuiCol_ResizeGrip] = ImVec4{ 0.6f, 0.2f, 0.8f, 1.0f };
		colors[ImGuiCol_ResizeGripHovered] = ImVec4{ 0.7f, 0.3f, 0.9f, 1.0f };
		colors[ImGuiCol_ResizeGripActive] = ImVec4{ 0.9f, 0.4f, 1.0f, 1.0f };

		// Separators
		colors[ImGuiCol_Separator] = ImVec4{ 0.4f, 0.0f, 0.5f, 1.0f };
		colors[ImGuiCol_SeparatorHovered] = ImVec4{ 0.5f, 0.1f, 0.6f, 1.0f };
		colors[ImGuiCol_SeparatorActive] = ImVec4{ 0.6f, 0.2f, 0.7f, 1.0f };
	}

	void ImGuiLayer::StyleColorsCyberpunk()
	{
		auto& colors = ImGui::GetStyle().Colors;

		// Background
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.05f, 0.05f, 0.1f, 1.0f }; // Deep Dark Blue

		// Headers
		colors[ImGuiCol_Header] = ImVec4{ 0.1f, 0.3f, 0.5f, 1.0f }; // Electric Blue
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.2f, 0.6f, 1.0f, 1.0f }; // Bright Neon Blue
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.9f, 0.0f, 0.9f, 1.0f }; // Cyberpunk Pink

		// Buttons
		colors[ImGuiCol_Button] = ImVec4{ 0.1f, 0.5f, 0.8f, 1.0f }; // Bright Blue
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.7f, 0.1f, 0.7f, 1.0f }; // Neon Pink
		colors[ImGuiCol_ButtonActive] = ImVec4{ 1.0f, 0.5f, 0.0f, 1.0f }; // Orange Glow

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4{ 0.1f, 0.2f, 0.3f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.3f, 0.7f, 1.0f, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.8f, 0.0f, 0.8f, 1.0f };

		// Title Bar
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.1f, 0.1f, 0.2f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.2f, 0.3f, 0.5f, 1.0f };
	}

	void ImGuiLayer::StyleColorsMidnightOcean()
	{
		auto& colors = ImGui::GetStyle().Colors;

		// Background
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.02f, 0.02f, 0.06f, 1.0f }; // Deep Navy

		// Headers
		colors[ImGuiCol_Header] = ImVec4{ 0.05f, 0.3f, 0.4f, 1.0f }; // Ocean Teal
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.1f, 0.5f, 0.6f, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.0f, 0.8f, 0.9f, 1.0f };

		// Buttons
		colors[ImGuiCol_Button] = ImVec4{ 0.05f, 0.3f, 0.4f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.1f, 0.5f, 0.6f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.0f, 0.8f, 0.9f, 1.0f };

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4{ 0.02f, 0.1f, 0.15f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.1f, 0.5f, 0.6f, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.0f, 0.8f, 0.9f, 1.0f };

		// Title Bar
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.02f, 0.1f, 0.15f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.1f, 0.5f, 0.6f, 1.0f };
	}

	void ImGuiLayer::StyleColorsRustGold()
	{
		auto& colors = ImGui::GetStyle().Colors;

		// Background
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.15f, 0.1f, 0.08f, 1.0f }; // Dark Brown

		// Headers
		colors[ImGuiCol_Header] = ImVec4{ 0.5f, 0.25f, 0.1f, 1.0f }; // Rust Orange
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.7f, 0.3f, 0.15f, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.9f, 0.4f, 0.2f, 1.0f }; // Golden Accent

		// Buttons
		colors[ImGuiCol_Button] = ImVec4{ 0.5f, 0.25f, 0.1f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.7f, 0.3f, 0.15f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.9f, 0.4f, 0.2f, 1.0f };

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4{ 0.2f, 0.15f, 0.1f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.7f, 0.3f, 0.15f, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.9f, 0.4f, 0.2f, 1.0f };

		// Title Bar
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.2f, 0.15f, 0.1f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.7f, 0.3f, 0.15f, 1.0f };
	}


}