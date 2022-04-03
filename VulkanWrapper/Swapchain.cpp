#include "Swapchain.h"
#include "Log.h"
#include "Image.h"

#include <algorithm>

namespace VulkanWrapper
{
    SwapChainSupport getSwapchainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
    {
        // check swap chain support
        SwapChainSupport swapchain_support{};

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapchain_support.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, {});
        if (formatCount == 0)
            return swapchain_support;

        swapchain_support.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapchain_support.formats.data());

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, {});
        if (presentModeCount == 0)
            return swapchain_support;

        swapchain_support.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, swapchain_support.presentModes.data());

        swapchain_support.suitable = true;

        return swapchain_support;
    }
    
    void Swapchain::init(const DeviceManager& device_manager, GLFWwindow* window, VkSurfaceKHR surface)
    {
        // Select a surface format
        if (device_manager.surface_formats.empty()) log_error("No surface formats");

        VkSurfaceFormatKHR surfaceFormat = device_manager.surface_formats[0];
        for (const auto& availableFormat : device_manager.surface_formats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                surfaceFormat = availableFormat;
        }

        // Select a present mode
        if (device_manager.surface_presentModes.empty()) log_error("No present modes");

        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
        for (const auto& availableMode : device_manager.surface_presentModes)
        {
            if (availableMode == VK_PRESENT_MODE_MAILBOX_KHR)
                presentMode = availableMode;
        }

        // Work out the extent
        {
            if (device_manager.surface_capabilities.currentExtent.width != UINT32_MAX)
            {
                extent = device_manager.surface_capabilities.currentExtent;
            }
            else
            {
                int framebuffer_width;
                int framebuffer_height;
                glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);

                VkExtent2D actualExtent = { static_cast<uint32_t>(framebuffer_width), static_cast<uint32_t>(framebuffer_height) };
                actualExtent.width = std::clamp(actualExtent.width, device_manager.surface_capabilities.minImageExtent.width, device_manager.surface_capabilities.maxImageExtent.width);
                actualExtent.height = std::clamp(actualExtent.height, device_manager.surface_capabilities.minImageExtent.height, device_manager.surface_capabilities.maxImageExtent.height);
                
                extent = actualExtent;
            }
        }

        uint32_t image_count = device_manager.surface_capabilities.minImageCount + 1;
        if (device_manager.surface_capabilities.maxImageCount > 0)
            image_count = std::min(image_count, device_manager.surface_capabilities.maxImageCount);

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = image_count;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queue_family_indices[] = { device_manager.graphicsQueueFamily, device_manager.presentQueueFamily };
        if (device_manager.graphicsQueueFamily != device_manager.presentQueueFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queue_family_indices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = device_manager.surface_capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device_manager.logicalDevice, &createInfo, nullptr, &handle) != VK_SUCCESS)
            log_error("Failed to create swapchain!");

        vkGetSwapchainImagesKHR(device_manager.logicalDevice, handle, &image_count, nullptr);
        swapChainImages.resize(image_count);
        vkGetSwapchainImagesKHR(device_manager.logicalDevice, handle, &image_count, swapChainImages.data());

        swapChainImageViews.resize(image_count);
        for (size_t i = 0; i < image_count; i++)
        {
            swapChainImageViews[i] = createImageView(device_manager.logicalDevice, swapChainImages[i], surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }

        imageFormat = surfaceFormat.format;
    }

    void Swapchain::deinit(const DeviceManager& device_manager)
    {
        for (auto imageView : swapChainImageViews)
            vkDestroyImageView(device_manager.logicalDevice, imageView, nullptr);

        vkDestroySwapchainKHR(device_manager.logicalDevice, handle, nullptr);
    }
}