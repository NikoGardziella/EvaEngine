#include "pch.h"
#include "VulkanRenderer2D.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"
#include <Engine/Platform/Vulkan/VulkanGraphicsPipeline.h>
#include "VertexArray.h"
#include "Shader.h"
#include "Renderer2D.cpp"

namespace Engine {

	struct VulkanRenderer2DData
	{
		static const uint32_t MaxQuads = 20000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32; // TODO: RenderCaps


		Ref<VertexArray> LineVertexArray;
		Ref<VertexBuffer> LineVertexBuffer;
		Ref<Shader> LineShader;

		uint32_t LineVertexCount = 0;
		LineVertex* LineVertexBufferBase = nullptr;
		LineVertex* LineVertexBufferPtr = nullptr;
		float LineWidth = 2.0f;


		Ref<VertexArray> CircleVertexArray;
		Ref<VertexBuffer> CircleVertexBuffer;
		Ref<Shader> CircleShader;

		uint32_t CircleIndexCount = 0;
		CircleVertex* CircleVertexBufferBase = nullptr;
		CircleVertex* CircleVertexBufferPtr = nullptr;


		Ref<VertexArray> QuadVertexArray;
		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<Shader> QuadShader;
		Ref<Texture2D> WhiteTexture;

		uint32_t QuadIndexCount = 0;
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPtr = nullptr;

		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 = white texture

		glm::vec4 QuadVertexPositions[4];

		Renderer2D::Statistics Stats;

		struct CameraData
		{
			glm::mat4 ViewProjection;
		};
		CameraData CameraBuffer;
		Ref<UniformBuffer> CameraUniformBuffer;
	};


	static VulkanRenderer2DData s_VulkanData;
	std::shared_ptr<Engine::VulkanGraphicsPipeline> Engine::VulkanRenderer2D::m_pipeline = nullptr;

	void VulkanRenderer2D::Init()
	{
		VulkanContext* vulkanContext = VulkanContext::Get();
		m_pipeline = std::make_shared<VulkanGraphicsPipeline>(vulkanContext->GetDeviceManager().GetDevice(),
			vulkanContext->GetSwapchain().GetSwapchainExtent(), vulkanContext->GetRenderPass());
	}
}
