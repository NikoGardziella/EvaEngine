#include "pch.h"
#include "VulkanSharedResources.h"
#include <memory>
#include "Lights/Shadow/VulkanShadowMap.h"


namespace Engine {


	void Engine::VulkanSharedResources::Init(VulkanContext* ctx)
	{

		m_shadowMap = std::make_shared<VulkanShadowMap>();
		EE_CORE_INFO("shadow map created");

		m_shadowMap->InitShadowMap(ctx->GetDeviceManager().GetDevice(), ctx->GetDeviceManager().GetPhysicalDevice());




	}
}
