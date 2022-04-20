#include "Image.h"
#include "Log.h"

namespace VulkanWrapper
{
    static uint32_t findMemoryType(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties mem_properties;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);
        for (uint32_t i{}; i < mem_properties.memoryTypeCount; ++i)
        {
            if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }

        log_error("failed to find suitable memory type!");
    }

    void Image::createImage(
        VkDevice logical_device,
        VkPhysicalDevice physical_device,
        const uint32_t width,
        const uint32_t height,
        uint32_t mip_map_levels,
        VkSampleCountFlagBits num_samples,
        const VkFormat format,
        const VkImageTiling tiling,
        const VkImageUsageFlags usage)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mip_map_levels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = num_samples;
        imageInfo.flags = 0;

        if (vkCreateImage(logical_device, &imageInfo, nullptr, &handle) != VK_SUCCESS)
            log_error("failed to create image!");

        VkMemoryRequirements mem_requirements;
        vkGetImageMemoryRequirements(logical_device, handle, &mem_requirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = mem_requirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(physical_device, mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(logical_device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
            log_error("failed to allocate image memory");

        vkBindImageMemory(logical_device, handle, memory, 0);
    }

    void Image::deinit(VkDevice logical_device) {
        vkDestroyImageView(logical_device, view, nullptr);
        vkDestroyImage(logical_device, handle, nullptr);
        vkFreeMemory(logical_device, memory, nullptr);
    }

    VkImageView createImageView(VkDevice logical_device, VkImage image_handle, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_map_levels )
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = image_handle;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = aspect_flags;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = mip_map_levels;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkImageView image_view;
        if (vkCreateImageView(logical_device, &createInfo, nullptr, &image_view) != VK_SUCCESS)
            log_error("Failed to create swapchain image views!");

        return image_view;
    }
}