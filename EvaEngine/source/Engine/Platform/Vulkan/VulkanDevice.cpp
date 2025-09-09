#include "pch.h"
#include "VulkanDevice.h"
#include "VulkanUtils.h"

namespace Engine {

    VulkanDevice::VulkanDevice(VkInstance instance, VkSurfaceKHR surface, bool m_enableValidationLayers)
		: m_instance(instance), m_surface(surface), m_enableValidationLayers(m_enableValidationLayers)
    {
        PickPhysicalDevice();
        CreateLogicalDevice();

    }

    void VulkanDevice::PickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
        if (deviceCount == 0)
        {
            EE_CORE_ERROR("Failed to find GPUs with Vulkan support!");
            return;
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

        for (const auto& device : devices)
        {
            if (IsDeviceSuitable(device))
            {
                VkPhysicalDeviceProperties deviceProperties;
                vkGetPhysicalDeviceProperties(device, &deviceProperties);
                EE_CORE_INFO("Found device: {}", deviceProperties.deviceName);
                EE_CORE_INFO("maxPerStageDescriptorStorageImages = {}", deviceProperties.limits.maxPerStageDescriptorStorageImages);
                EE_CORE_INFO("maxPerStageResources             = {}", deviceProperties.limits.maxPerStageResources);
                EE_CORE_INFO("maxDescriptorSetStorageImages    = {}", deviceProperties.limits.maxDescriptorSetStorageImages);

                m_physicalDevice = device;
                break;
            }
        }


        if (m_physicalDevice == VK_NULL_HANDLE)
        {
            EE_CORE_ERROR("Failed to find a suitable GPU!");
            return;
        }
    }

    void VulkanDevice::CreateLogicalDevice()
    {
        auto indices = VulkanUtils::FindQueueFamilies(m_physicalDevice, m_surface);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {
            indices.graphicsFamily.value(),
            indices.presentFamily.value()
        };

        float queuePriority = 1.0f;
        for (uint32_t qf : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo q{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
            q.queueFamilyIndex = qf;
            q.queueCount = 1;
            q.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(q);
        }

        // ----- Version gate: 1.2 core vs 1.1 + EXT -----
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
        const bool hasVk12 =
            VK_API_VERSION_MAJOR(props.apiVersion) > 1 ||
            (VK_API_VERSION_MAJOR(props.apiVersion) == 1 && VK_API_VERSION_MINOR(props.apiVersion) >= 2);

        // If you truly DON’T use update-after-bind, keep these false.
        const bool wantUpdateAfterBind = false;

        // Feature chain root
        VkPhysicalDeviceFeatures2 features2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        features2.features = m_deviceFeatures; // keep your old core toggles (samplerAnisotropy etc.)

        // 1.2 core path
        VkPhysicalDeviceVulkan12Features f12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        // 1.1 + EXT path
        VkPhysicalDeviceDescriptorIndexingFeatures fExt{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES
        };

        if (hasVk12)
        {
            f12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
            f12.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
            f12.descriptorBindingPartiallyBound = VK_TRUE;
            f12.runtimeDescriptorArray = VK_TRUE;
            if (wantUpdateAfterBind)
            {
                f12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
                f12.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
            }
            features2.pNext = &f12;
        }
        else 
        {
            // Ensure the device extension list contains VK_EXT_descriptor_indexing
            if (std::find(m_deviceExtensions.begin(), m_deviceExtensions.end(),
                VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME) == m_deviceExtensions.end())
            {
                m_deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
            }

            fExt.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
            fExt.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
            fExt.descriptorBindingPartiallyBound = VK_TRUE;
            fExt.runtimeDescriptorArray = VK_TRUE;
            if (wantUpdateAfterBind)
            {
                fExt.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
                fExt.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
            }
            features2.pNext = &fExt;
        }

        // (Optional) probe support and assert
        {
            VkPhysicalDeviceFeatures2 probe{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
            void* tail = nullptr;
            if (hasVk12) {
                VkPhysicalDeviceVulkan12Features supp{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
                tail = &supp; probe.pNext = tail;
                vkGetPhysicalDeviceFeatures2(m_physicalDevice, &probe);
                EE_CORE_ASSERT(supp.shaderSampledImageArrayNonUniformIndexing, "Non-uniform sampled indexing not supported");
                EE_CORE_ASSERT(supp.shaderStorageImageArrayNonUniformIndexing, "Non-uniform storage indexing not supported");
                EE_CORE_ASSERT(supp.descriptorBindingPartiallyBound, "Partially-bound not supported");
                EE_CORE_ASSERT(supp.runtimeDescriptorArray, "Runtime descriptor array not supported");
            }
            else
            {
                VkPhysicalDeviceDescriptorIndexingFeatures supp{
                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES
                };
                tail = &supp; probe.pNext = tail;
                vkGetPhysicalDeviceFeatures2(m_physicalDevice, &probe);
                EE_CORE_ASSERT(supp.shaderSampledImageArrayNonUniformIndexing, "Non-uniform sampled indexing not supported (EXT)");
                EE_CORE_ASSERT(supp.shaderStorageImageArrayNonUniformIndexing, "Non-uniform storage indexing not supported (EXT)");
                EE_CORE_ASSERT(supp.descriptorBindingPartiallyBound, "Partially-bound not supported (EXT)");
                EE_CORE_ASSERT(supp.runtimeDescriptorArray, "Runtime descriptor array not supported (EXT)");
            }
        }

        // Device create info
        VkDeviceCreateInfo createInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        createInfo.pNext = &features2; // IMPORTANT: use Features2 chain
        createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = nullptr;          // do NOT also set this
        createInfo.enabledExtensionCount = (uint32_t)m_deviceExtensions.size();
        createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

        if (m_enableValidationLayers)
        {
            createInfo.enabledLayerCount = (uint32_t)m_validationLayers.size();
            createInfo.ppEnabledLayerNames = m_validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
        {
            EE_CORE_ERROR("Failed to create logical device");
        }

        // Queues
        vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
    }


    bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice device)
    {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        VulkanContext::QueueFamilyIndices indices = VulkanUtils::FindQueueFamilies(device, m_surface);
        bool extensionsSupported = CheckDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported)
        {
            VulkanContext::SwapChainSupportDetails swapChainSupport = VulkanUtils::QuerySwapChainSupport(device, m_surface);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        if (!m_deviceFeatures.wideLines)
        {
            m_deviceFeatures.wideLines = VK_TRUE;
        }
        else
        {
            EE_CORE_WARN("wideLines not supported on this GPU!");
        }
        if (!m_deviceFeatures.fragmentStoresAndAtomics) 
        {
            m_deviceFeatures.fragmentStoresAndAtomics = VK_TRUE;
        }
        else
        {
            EE_CORE_WARN("fragmentStoresAndAtomics not supported on this GPU!");
        }
        bool hasVk12 = (VK_VERSION_MAJOR(deviceProperties.apiVersion) > 1) ||
            (VK_VERSION_MAJOR(deviceProperties.apiVersion) == 1 && VK_VERSION_MINOR(deviceProperties.apiVersion) >= 2);

        EE_CORE_ASSERT(hasVk12, "Device does not expose Vulkan 1.2; use Option B with the EXT.");

        
        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device)
    {
        

        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());

        for (const auto& extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
            //EE_CORE_INFO("extension: {}", extension.extensionName);
        }

        return requiredExtensions.empty();
    }

}
