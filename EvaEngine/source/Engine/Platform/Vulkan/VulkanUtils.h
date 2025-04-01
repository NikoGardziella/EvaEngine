#pragma once
#include "VulkanContext.h"

namespace Engine {

	namespace VulkanUtils
	{

		VulkanContext::QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
		VulkanContext::SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);



	}

	

	
}


