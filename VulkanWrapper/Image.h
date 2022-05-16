#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace VulkanWrapper
{
    struct DeviceManager;

    struct Image
    {
        VkImage handle;
        VkDeviceMemory memory;
        VkImageView view;

        uint32_t width;
        uint32_t height;
        uint32_t mip_map_levels;
        VkFormat format;

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

    void createImageView(VkDevice logical_device, Image& image, VkImageAspectFlags aspect_flags);

    void uploadTextureData(const DeviceManager& device_manager, Image& image);
}