#include "DeviceManager.h"
#include "Swapchain.h"
#include "Log.h"

#include <set>
#include <optional>
#include <array>
#include <string>

namespace VulkanWrapper
{
    struct CandidateDeviceSettings
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        SwapChainSupport swapchain_support;

        VkFormat depth_format = VK_FORMAT_UNDEFINED;

        bool suitable = false;
    };

    CandidateDeviceSettings isDeviceSuitable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
    {
        CandidateDeviceSettings deviceSettings;

        // basic device properties
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        // optional features support (e.g. texture compression, multi viewport rendering)
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);

        {
            // find queue families
            uint32_t queueFamilyCount{};
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, {});
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

            for (int i = 0; i < queueFamilies.size(); ++i)
            {
                if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    deviceSettings.graphicsFamily = i;

                VkBool32 presentSupport{};
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
                if (presentSupport)
                    deviceSettings.presentFamily = i;

                if (!deviceSettings.graphicsFamily || !deviceSettings.presentFamily)
                    return deviceSettings;
            }
        }

        {
            // check device extension support
            uint32_t extensionCount{};
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> availableExtensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

            std::set<std::string> requiredExtensions(device_extensions.begin(), device_extensions.end());
            for (const auto& extension : availableExtensions)
                requiredExtensions.erase(extension.extensionName);

            if (!requiredExtensions.empty())
                return deviceSettings;
        }

        {
            // check swapchain support
            deviceSettings.swapchain_support = getSwapchainSupport(device, surface);
            if (!deviceSettings.swapchain_support.suitable)
                return deviceSettings;
        }

        {
            // check depth format support
            constexpr std::array<VkFormat, 3> candidates = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
            for (VkFormat format : candidates)
            {
                VkFormatProperties props;
                vkGetPhysicalDeviceFormatProperties(device, format, &props);

                if (VK_IMAGE_TILING_OPTIMAL == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
                {
                    deviceSettings.depth_format = format;
                    break;
                }
                else if (VK_IMAGE_TILING_OPTIMAL == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
                {
                    deviceSettings.depth_format = format;
                    break;
                }
            }

            if (deviceSettings.depth_format == VK_FORMAT_UNDEFINED)
                return deviceSettings;
        }

        deviceSettings.suitable = true;
        return deviceSettings;
    };

    void DeviceManager::init(VkInstance& instance, VkSurfaceKHR& surface)
    {
        {
            // select physical device
            uint32_t deviceCount{};
            vkEnumeratePhysicalDevices(instance, &deviceCount, {});

            if (!deviceCount)
                log_error("failed to find gpus with Vulkan support!");

            std::vector<VkPhysicalDevice> devices(deviceCount);
            vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

            // TODO: pick best device instead of just first
            for (auto device : devices)
            {
                auto deviceSettings = isDeviceSuitable(device, surface);
                if (deviceSettings.suitable)
                {
                    physicalDevice = device;
                    graphicsQueueFamily = deviceSettings.graphicsFamily.value();
                    presentQueueFamily = deviceSettings.presentFamily.value();

                    surface_capabilities = deviceSettings.swapchain_support.capabilities;
                    surface_formats = deviceSettings.swapchain_support.formats;
                    surface_presentModes = deviceSettings.swapchain_support.presentModes;
                    depth_format = deviceSettings.depth_format;
                    
                    msaaSamples = [&device]() {
                        VkPhysicalDeviceProperties physicalDeviceProperties;
                        vkGetPhysicalDeviceProperties(device, &physicalDeviceProperties);

                        VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
                        if (counts & VK_SAMPLE_COUNT_64_BIT)
                            return VK_SAMPLE_COUNT_64_BIT;
                        else if (counts & VK_SAMPLE_COUNT_32_BIT)
                            return VK_SAMPLE_COUNT_32_BIT;
                        else if (counts & VK_SAMPLE_COUNT_16_BIT)
                            return VK_SAMPLE_COUNT_16_BIT;
                        else if (counts & VK_SAMPLE_COUNT_8_BIT)
                            return VK_SAMPLE_COUNT_8_BIT;
                        else if (counts & VK_SAMPLE_COUNT_4_BIT)
                            return VK_SAMPLE_COUNT_4_BIT;
                        else if (counts & VK_SAMPLE_COUNT_2_BIT)
                            return VK_SAMPLE_COUNT_2_BIT;

                        return VK_SAMPLE_COUNT_1_BIT;
                    }();

                    break;
                }
            }

            if (physicalDevice == VK_NULL_HANDLE)
                log_error("failed to find suitable gpu");
        }

        {
            // create logical device
            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
            std::set<uint32_t> uniqueQueueFamilies = { graphicsQueueFamily, presentQueueFamily };

            const float queuePriority = 1.0f;
            for (auto queueFamily : uniqueQueueFamilies)
            {
                VkDeviceQueueCreateInfo queueCreateInfo{};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = queueFamily;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &queuePriority;
                queueCreateInfos.push_back(queueCreateInfo);
            }

            VkPhysicalDeviceFeatures deviceFeatures{};
            deviceFeatures.samplerAnisotropy = VK_TRUE;
            deviceFeatures.sampleRateShading = VK_TRUE; // enable sample shading to stop alisaing within textures

            VkDeviceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            createInfo.pQueueCreateInfos = queueCreateInfos.data();
            createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
            createInfo.pEnabledFeatures = &deviceFeatures;

            createInfo.enabledExtensionCount = (uint32_t)device_extensions.size();
            createInfo.ppEnabledExtensionNames = device_extensions.data();

            // only valid for older versions of vulkan
            if (enable_validation_layers)
            {
                createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
                createInfo.ppEnabledLayerNames = validation_layers.data();
            }
            else
                createInfo.enabledLayerCount = 0;
           
            createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
            createInfo.pQueueCreateInfos = queueCreateInfos.data();

            if (vkCreateDevice(physicalDevice, &createInfo, {}, &logicalDevice) != VK_SUCCESS)
                log_error("failed to create logical device!");

            vkGetDeviceQueue(logicalDevice, graphicsQueueFamily, 0, &graphicsQueue);
            vkGetDeviceQueue(logicalDevice, presentQueueFamily, 0, &presentQueue);
        }

        // create command pool
        {
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.queueFamilyIndex = graphicsQueueFamily;
            poolInfo.flags = 0;

            if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &command_pool) != VK_SUCCESS)
                log_error("failed to create command pool!");
        }
    }

    void DeviceManager::deinit()
    {
        vkDestroyCommandPool(logicalDevice, command_pool, nullptr);

        vkDestroyDevice(logicalDevice, nullptr);
    }
}