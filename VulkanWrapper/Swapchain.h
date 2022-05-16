#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "DeviceManager.h"
#include "Image.h"

namespace VulkanWrapper
{
    struct SwapChainSupport
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;

        bool suitable = false;
    };

    SwapChainSupport getSwapchainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);

    struct Swapchain
    {
        VkExtent2D extent;
        VkSwapchainKHR handle;

        std::vector<Image> images;
        VkFormat image_format;

        void init(const DeviceManager& device_manager, GLFWwindow* window, VkSurfaceKHR surface);
        void deinit(const DeviceManager& device_manager);
    };
}