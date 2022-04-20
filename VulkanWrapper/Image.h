#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace VulkanWrapper
{
    struct Image
    {
        VkImage handle;
        VkDeviceMemory memory;
        VkImageView view;

        void createImage(
            VkDevice logical_device,
            VkPhysicalDevice physical_device,
            const uint32_t width,
            const uint32_t height,
            uint32_t mipMapLevels,
            VkSampleCountFlagBits numSamples,
            const VkFormat format,
            const VkImageTiling tiling,
            const VkImageUsageFlags usage);

        void deinit(VkDevice logical_device);
    };

    VkImageView createImageView(VkDevice logical_device, VkImage image_handle, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_map_levels );
}