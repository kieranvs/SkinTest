#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "DeviceManager.h"

namespace VulkanWrapper
{
    struct SwapChainSupport
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;

        bool suitable = false;
    };

    SwapChainSupport getSwapchainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);

    struct Swapchain
    {
        VkExtent2D extent;
        VkSwapchainKHR handle;

        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;
        VkFormat imageFormat;

        void init(const DeviceManager& device_manager, GLFWwindow* window, VkSurfaceKHR surface);
        void deinit(const DeviceManager& device_manager);
    };
}