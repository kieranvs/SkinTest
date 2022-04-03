#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

namespace VulkanWrapper
{
    const bool enable_validation_layers = true;
    const std::vector<const char*> validation_layers = { "VK_LAYER_KHRONOS_validation" };
    const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    struct DeviceManager
    {
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        uint32_t graphicsQueueFamily;
        uint32_t presentQueueFamily;
        VkQueue graphicsQueue;
        VkQueue presentQueue;

        VkCommandPool command_pool;

        VkDevice logicalDevice;

        VkSurfaceCapabilitiesKHR surface_capabilities;
        std::vector<VkSurfaceFormatKHR> surface_formats;
        std::vector<VkPresentModeKHR> surface_presentModes;
        VkFormat depth_format;

        VkSampleCountFlagBits msaaSamples;

        void init(VkInstance& instance, VkSurfaceKHR& surface);
        void deinit();
    };
}