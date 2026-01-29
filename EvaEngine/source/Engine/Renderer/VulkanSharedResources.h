#pragma once
#include <Engine/Platform/Vulkan/VulkanContext.h>
#include <Engine/Core/Core.h>
#include "Lights/Shadow/VulkanShadowMap.h"

namespace Engine {

	class VulkanSharedResources {
	public:
		void Init(VulkanContext* ctx);
		void Shutdown();

		Ref<VulkanShadowMap>& GetShadowMap() { return m_shadowMap; }

	private:
		Ref<VulkanShadowMap> m_shadowMap;
	};

}