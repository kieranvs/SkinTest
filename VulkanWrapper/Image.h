#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace VulkanWrapper
{
    struct Image
    {
        VkImage image;
        VkDeviceMemory image_memory;
        VkImageView image_view;

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

    VkImageView createImageView(VkDevice logical_device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipMapLevels );
}